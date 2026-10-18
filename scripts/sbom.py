#!/usr/bin/env python3
"""Generate Jinja2C++'s SBOMs from sbom/components.json (docs/tasks/0187).

sbom/components.json is the one hand-edited inventory of third-party code and data.
This script turns it into CycloneDX 1.6 and SPDX 2.3 JSON documents, in two profiles:

  library  what goes into the library and its install: mandatory and alternative
           dependencies, vendored code, generated data, and Boost libraries that are
           pulled in but unused (installed with the package, so they are listed)
  full     everything, including tests, benchmarks and developer/CI tooling

The library profile is installed with the package (share/doc/jinja2cpp/sbom/). The
classification it carries (role, phases, exposure, delivery) is explained in
docs/dependencies.md and travels as CycloneDX properties and SPDX package comments.

Usage: scripts/sbom.py [--check] [--validate]
  (no flag)   rewrite the four sbom/jinja2cpp*.json files
  --check     exit 1 when a file is stale, a pin in components.json no longer matches the
              build files, or the build uses a dependency components.json does not list
  --validate  also validate the documents with cyclonedx-python-lib and spdx-tools (CI
              installs pinned versions; not needed to generate)

Standard library only, so it runs wherever Python 3.8+ does. Output is deterministic:
no timestamps beyond `created` in components.json, ids derived from the content.
"""
import argparse
import fnmatch
import hashlib
import json
import pathlib
import re
import sys
import uuid

ROOT = pathlib.Path(__file__).resolve().parent.parent
SBOM = ROOT / "sbom"
MANIFEST = SBOM / "components.json"
PROPERTY = "jinja2cpp:"

ROLES = {"mandatory", "alternative", "optional", "test", "tooling", "redundant"}
PHASES = {"build", "consumer-build", "consumer-link", "runtime", "test", "dev", "ci"}
EXPOSURES = {"public-api", "public-optional", "link", "internal", "none"}
DELIVERIES = {"fetched", "vendored", "generated", "system", "ci"}


def load():
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    by_id = {}
    for comp in data["components"]:
        if comp["id"] in by_id:
            raise SystemExit(f"components.json: duplicate id {comp['id']}")
        by_id[comp["id"]] = comp
    for comp in data["components"]:
        parent = by_id.get(comp.get("parent"))
        if comp.get("parent") and not parent:
            raise SystemExit(f"components.json: {comp['id']} has unknown parent {comp['parent']}")
        if parent:
            # A sub-component is a library inside its parent's distribution
            for key in ("version", "license", "supplier", "url", "delivery", "selected_by"):
                comp.setdefault(key, parent.get(key))
            lib = comp["name"][len(parent["name"]) + 1:]
            comp.setdefault("purl", parent["purl"].replace("/boost@", f"/{lib}@"))
        comp.setdefault("selected_by", "always")
    return data, by_id


def root_version(root):
    spec = root["version_from"]
    text = (ROOT / spec["file"]).read_text(encoding="utf-8")
    match = re.search(spec["regex"], text)
    if not match:
        raise SystemExit(f"cannot read the project version from {spec['file']}")
    return match.group(1)


def in_profile(comp, profile):
    if profile == "full":
        return True
    if comp["role"] in ("mandatory", "alternative"):
        return True
    return comp["role"] == "redundant" and comp.get("parent") is not None


def file_hashes(comp):
    return [(path, hashlib.sha256((ROOT / path).read_bytes()).hexdigest()) for path in comp.get("files", [])]


def classification(comp):
    props = [
        ("role", comp["role"]),
        ("phases", ",".join(comp["phases"])),
        ("exposure", comp["exposure"]),
        ("delivery", comp["delivery"]),
        ("selected-by", comp["selected_by"]),
    ]
    if comp.get("also_roles"):
        props.append(("also-roles", ",".join(comp["also_roles"])))
    for key in ("used_for", "why", "lighten", "modified", "conan"):
        if comp.get(key):
            props.append((key.replace("_", "-"), comp[key]))
    for path, digest in file_hashes(comp):
        props.append(("file", f"{path} sha256:{digest}"))
    for path in comp.get("generators", []):
        props.append(("generator", path))
    if comp.get("transitive_libraries"):
        props.append(("transitive-libraries", ",".join(comp["transitive_libraries"])))
    return props


def cdx_licenses(expr):
    if expr == "NOASSERTION":
        return []
    if " " in expr:
        return [{"expression": expr}]
    return [{"license": {"id": expr}}]


def cdx_scope(comp):
    if comp["role"] == "mandatory":
        return "required"
    if comp["role"] in ("alternative", "optional"):
        return "optional"
    return "excluded"


def cdx_type(comp):
    if comp["delivery"] == "generated":
        return "data"
    if comp["role"] == "tooling" and comp["delivery"] != "fetched":
        return "application"
    return "library"


def cdx_component(comp, children):
    out = {"type": cdx_type(comp), "bom-ref": comp["id"], "name": comp["name"]}
    if comp.get("version") and comp["version"] != "NOASSERTION":
        out["version"] = comp["version"]
    if comp.get("supplier"):
        out["supplier"] = {"name": comp["supplier"]}
    out["scope"] = cdx_scope(comp)
    hashes = [{"alg": "SHA-256", "content": comp["source"]["sha256"]}] if comp.get("source", {}).get("sha256") else []
    if len(comp.get("files", [])) == 1:
        hashes.append({"alg": "SHA-256", "content": file_hashes(comp)[0][1]})
    if hashes:
        out["hashes"] = hashes
    licenses = cdx_licenses(comp["license"])
    if licenses:
        out["licenses"] = licenses
    if comp.get("purl"):
        out["purl"] = comp["purl"]
    refs = []
    if comp.get("url"):
        refs.append({"type": "website", "url": comp["url"]})
    source = comp.get("source", {})
    if source.get("url"):
        refs.append({"type": "distribution", "url": source["url"]})
    if source.get("git"):
        refs.append({"type": "vcs", "url": source["git"], "comment": source.get("tag") or source.get("commit")})
    if refs:
        out["externalReferences"] = refs
    if comp.get("modified") or comp.get("patches"):
        pedigree = {}
        if comp.get("patches"):
            pedigree["patches"] = [{"type": "unofficial", "diff": {"url": path}} for path in comp["patches"]]
        if comp.get("modified"):
            pedigree["notes"] = comp["modified"]
        out["pedigree"] = pedigree
    out["properties"] = [{"name": PROPERTY + key, "value": value} for key, value in classification(comp)]
    if children:
        out["components"] = children
    return out


def cyclonedx(data, profile, version):
    root = data["root"]
    comps = [c for c in data["components"] if in_profile(c, profile)]
    kids = {}
    for comp in comps:
        if comp.get("parent"):
            kids.setdefault(comp["parent"], []).append(comp)
    top = [c for c in comps if not c.get("parent")]
    components = [cdx_component(c, [cdx_component(k, None) for k in kids.get(c["id"], [])]) for c in top]
    deps = [{"ref": root["id"], "dependsOn": [c["id"] for c in top]}]
    for parent, children in kids.items():
        deps.append({"ref": parent, "dependsOn": [k["id"] for k in children]})
    body = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {
            "timestamp": data["created"],
            "tools": {"components": [{"type": "application", "name": "scripts/sbom.py"}]},
            "component": {
                "type": "library",
                "bom-ref": root["id"],
                "name": root["name"],
                "version": version,
                "licenses": cdx_licenses(root["license"]),
                "purl": f"{root['purl_base']}@{version}",
                "externalReferences": [{"type": "vcs", "url": root["url"]}],
            },
            "properties": [{"name": PROPERTY + "profile", "value": profile}],
        },
        "components": components,
        "dependencies": deps,
    }
    digest = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()
    return {"bomFormat": "CycloneDX", "specVersion": "1.6",
            "serialNumber": f"urn:uuid:{uuid.uuid5(uuid.NAMESPACE_URL, root['url'] + '#' + digest)}",
            **{k: v for k, v in body.items() if k not in ("bomFormat", "specVersion")}}


def spdx_id(comp_id):
    return "SPDXRef-Package-" + re.sub(r"[^A-Za-z0-9.-]", "-", comp_id)


def spdx_relationship(comp, root_ref):
    ref = spdx_id(comp["id"])
    if comp.get("parent"):
        return (spdx_id(comp["parent"]), "CONTAINS", ref)
    if comp["delivery"] in ("vendored", "generated"):
        return (root_ref, "CONTAINS", ref)
    if comp["role"] == "mandatory":
        return (root_ref, "DEPENDS_ON", ref)
    if comp["role"] in ("alternative", "optional"):
        return (ref, "OPTIONAL_DEPENDENCY_OF", root_ref)
    if comp["role"] == "test":
        return (ref, "TEST_DEPENDENCY_OF", root_ref)
    if "build" in comp["phases"]:
        return (ref, "BUILD_TOOL_OF", root_ref)
    return (ref, "DEV_TOOL_OF", root_ref)


def spdx_package(comp):
    source = comp.get("source", {})
    if source.get("url"):
        location = source["url"]
    elif source.get("git"):
        location = f"git+{source['git']}@{source.get('tag') or source.get('commit')}"
    else:
        location = "NOASSERTION"
    out = {"SPDXID": spdx_id(comp["id"]), "name": comp["name"]}
    if comp.get("version") and comp["version"] != "NOASSERTION":
        out["versionInfo"] = comp["version"]
    out["supplier"] = f"Organization: {comp['supplier']}" if comp.get("supplier") else "NOASSERTION"
    out["downloadLocation"] = location
    out["filesAnalyzed"] = False
    checksums = [{"algorithm": "SHA256", "checksumValue": source["sha256"]}] if source.get("sha256") else []
    if len(comp.get("files", [])) == 1:
        checksums.append({"algorithm": "SHA256", "checksumValue": file_hashes(comp)[0][1]})
    if checksums:
        out["checksums"] = checksums
    if comp.get("url"):
        out["homepage"] = comp["url"]
    out["licenseConcluded"] = "NOASSERTION"
    out["licenseDeclared"] = comp["license"]
    out["copyrightText"] = "NOASSERTION"
    if comp.get("purl"):
        out["externalRefs"] = [{"referenceCategory": "PACKAGE-MANAGER", "referenceType": "purl",
                                "referenceLocator": comp["purl"]}]
    out["primaryPackagePurpose"] = {"generated": "FILE", "vendored": "SOURCE"}.get(
        comp["delivery"], "APPLICATION" if cdx_type(comp) == "application" else "LIBRARY")
    out["comment"] = "; ".join(f"{key}: {value}" for key, value in classification(comp))
    return out


def spdx(data, profile, version):
    root = data["root"]
    root_ref = spdx_id(root["id"])
    comps = [c for c in data["components"] if in_profile(c, profile)]
    packages = [{
        "SPDXID": root_ref, "name": root["name"], "versionInfo": version,
        "supplier": "Organization: Jinja2C++ project", "downloadLocation": f"git+{root['url']}@{version}",
        "filesAnalyzed": False, "homepage": root["url"], "licenseConcluded": root["license"],
        "licenseDeclared": root["license"], "copyrightText": "NOASSERTION",
        "externalRefs": [{"referenceCategory": "PACKAGE-MANAGER", "referenceType": "purl",
                          "referenceLocator": f"{root['purl_base']}@{version}"}],
        "primaryPackagePurpose": "LIBRARY",
    }] + [spdx_package(c) for c in comps]
    rels = [("SPDXRef-DOCUMENT", "DESCRIBES", root_ref)] + [spdx_relationship(c, root_ref) for c in comps]
    body = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{root['name']}-{version}-{profile}",
        "creationInfo": {"created": data["created"],
                         "creators": ["Tool: scripts/sbom.py", "Organization: Jinja2C++ project"]},
        "packages": packages,
        "relationships": [{"spdxElementId": a, "relationshipType": t, "relatedSpdxElement": b} for a, t, b in rels],
    }
    digest = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()[:16]
    body["documentNamespace"] = f"{root['url']}/sbom/{root['name']}-{version}-{profile}-{digest}"
    return body


def outputs(data):
    version = root_version(data["root"])
    files = {}
    for profile, suffix in (("library", ""), ("full", "-full")):
        files[SBOM / f"jinja2cpp{suffix}.cdx.json"] = cyclonedx(data, profile, version)
        files[SBOM / f"jinja2cpp{suffix}.spdx.json"] = spdx(data, profile, version)
    return {path: json.dumps(doc, indent=2, ensure_ascii=False) + "\n" for path, doc in files.items()}


def repo_files(*patterns):
    skip = ("build", ".git", ".claude")
    for pattern in patterns:
        for path in ROOT.glob(pattern):
            rel = path.relative_to(ROOT)
            if rel.parts and (rel.parts[0] in skip or rel.parts[0].startswith("build")):
                continue
            if path.is_file():
                yield rel.as_posix(), path


def audit(data, by_id):
    """What the build files say against what components.json says."""
    problems = []
    comps = data["components"]
    for comp in comps:
        for key, allowed in (("role", ROLES), ("exposure", EXPOSURES), ("delivery", DELIVERIES)):
            if comp.get(key) not in allowed:
                problems.append(f"{comp['id']}: {key} {comp.get(key)!r} is not one of {sorted(allowed)}")
        for phase in comp.get("phases", []):
            if phase not in PHASES:
                problems.append(f"{comp['id']}: phase {phase!r} is not one of {sorted(PHASES)}")
        for path in comp.get("files", []) + comp.get("generators", []) + comp.get("patches", []):
            if not (ROOT / path).is_file():
                problems.append(f"{comp['id']}: {path} does not exist")
        for item in comp.get("evidence", []):
            text = (ROOT / item["file"]).read_text(encoding="utf-8")
            for needle in item["contains"]:
                if needle not in text:
                    problems.append(f"{comp['id']}: {item['file']} no longer contains {needle!r}")

    # Every FetchContent source is a listed component
    sources = set()
    for comp in comps:
        source = comp.get("source", {})
        sources.update(v for v in (source.get("url"), source.get("git")) if v)
    for rel, path in repo_files("**/CMakeLists.txt", "**/*.cmake"):
        for block in re.findall(r"FetchContent_Declare\((.*?)\)", path.read_text(encoding="utf-8"), re.S):
            for url in re.findall(r"(?:URL|GIT_REPOSITORY)\s+(\S+)", block):
                if url not in sources:
                    problems.append(f"{rel}: FetchContent source {url} is not in components.json")

    # Every Conan requirement and every pip requirement
    conans = {c.get("conan") for c in comps}
    section = None
    for line in (ROOT / "conanfile.txt").read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith("["):
            section = line
        elif line and section == "[requires]" and line not in conans:
            problems.append(f"conanfile.txt: {line} is not the `conan` field of any component")
    purls = {c.get("purl") for c in comps}
    for rel, path in repo_files("**/requirements*.txt"):
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.split("#")[0].strip()
            if line:
                name, _, ver = line.partition("==")
                if f"pkg:pypi/{name.lower()}@{ver}" not in purls:
                    problems.append(f"{rel}: {line} is not in components.json")

    # Every GitHub Action outside this repository
    actions = set()
    for comp in comps:
        if comp.get("action"):
            actions.add(comp["action"])
            actions.update(comp.get("action_aliases", []))
    for rel, path in repo_files(".github/**/*.yml", ".github/**/*.yaml"):
        for match in re.finditer(r"^\s*(?:-\s*)?uses:\s*([^\s#]+)", path.read_text(encoding="utf-8"), re.M):
            used = match.group(1)
            if not used.startswith("./") and used not in actions:
                problems.append(f"{rel}: action {used} is not in components.json")

    # Every Boost header the code includes belongs to a listed Boost library
    headers = {h for c in comps for h in c.get("includes", [])}
    for rel, path in repo_files("src/**/*", "include/**/*", "test/**/*", "fuzz/**/*", "bench/**/*"):
        if path.suffix not in (".h", ".hpp", ".cpp", ".cc"):
            continue
        for header in re.findall(r"#\s*include\s*<(boost/[^>]+)>", path.read_text(encoding="utf-8", errors="replace")):
            if header not in headers:
                problems.append(f"{rel}: <{header}> is not in the `includes` of any boost-* component")

    # Every file kept in upstream formatting is listed as vendored code
    vendored = {p for c in comps if c["delivery"] == "vendored" for p in c.get("files", [])}
    ignore = ROOT / ".clang-format-ignore"
    for line in ignore.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        for rel, _ in repo_files(line):
            if rel not in vendored:
                problems.append(f".clang-format-ignore: {rel} is not a vendored component's file")
        if not any(fnmatch.fnmatch(v, line) for v in vendored):
            problems.append(f".clang-format-ignore: {line} matches no vendored file")
    return problems


def validate(texts):
    from cyclonedx.schema import SchemaVersion
    from cyclonedx.validation.json import JsonStrictValidator
    from spdx_tools.spdx.parser.parse_anything import parse_file
    from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document

    problems = []
    for path, text in texts.items():
        if path.name.endswith(".cdx.json"):
            error = JsonStrictValidator(SchemaVersion.V1_6).validate_str(text)
            if error:
                problems.append(f"{path.name}: {str(error).splitlines()[0]}")
        else:
            for message in validate_full_spdx_document(parse_file(str(path))):
                problems.append(f"{path.name}: {message.validation_message}")
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--validate", action="store_true")
    args = ap.parse_args()
    data, by_id = load()
    problems = audit(data, by_id)
    texts = outputs(data)
    for path, text in texts.items():
        if args.check:
            if not path.is_file() or path.read_text(encoding="utf-8") != text:
                problems.append(f"{path.relative_to(ROOT)} is stale: run scripts/sbom.py")
        else:
            path.write_text(text, encoding="utf-8")
    if args.validate:
        problems += validate(texts)
    for problem in problems:
        print(f"::error::{problem}" if args.check else problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
