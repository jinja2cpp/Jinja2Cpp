//! Rust template engines on the bench/cases workloads (docs/tasks/0135): MiniJinja, the
//! Jinja2 implementation by Jinja2's own author, and Tera 2, the Jinja2/Django-like engine.
//! The Rust counterpart of bench/engines_bench.cpp, driven the same way by bench/run.py
//! (`--engines`), so the two binaries take the same flags and print the same JSON.
//!
//! The same case directories as jinja2cpp_bench: main.j2, data.json, other *.j2 files as
//! includes, settings.json. Each engine gets the data in its own native form (a
//! `minijinja::Value`, a `tera::Context`), built once outside the timed loop, as
//! jinja2cpp_bench builds its ValuesMap once. Tera's dialect differs from Jinja's (keyword
//! arguments only, components instead of macros, no trim_blocks), so a case may carry
//! `<name>.tera` next to `<name>.j2`: the same template written for Tera, used in its place.
//! A case an engine cannot run (a settings.json option it lacks, a parse or render error)
//! is left out; bench/run.py also leaves out a case whose output differs from Python
//! Jinja2's, so every number in its table is the same work.
//!
//! Benchmarks: <engine>/Load/<case> (parse main.j2) and <engine>/Render/<case>.
//!
//! Usage: rust_engines_bench [--cases-dir=<dir>] [--dump-dir=<dir>] [--benchmark_filter=<regex>]
//!          [--benchmark_min_time=<seconds>s] [--benchmark_repetitions=<n>]
//!          [--benchmark_out=<file>] [--benchmark_out_format=json]
//!          [--benchmark_report_aggregates_only=true]
//!   --dump-dir  write <dir>/<engine>/<case>.txt for every case the engine renders (and
//!               <case>.err with the reason for one it does not) and exit
//! The --benchmark_* flags are the Google Benchmark flags bench/run.py passes; the JSON
//! has the fields it reads (median and cv aggregates with run_name, cpu_time in ns).

use std::collections::BTreeMap;
use std::fs;
use std::hint::black_box;
use std::path::{Path, PathBuf};

use serde_json::{json, Value as Json};

struct Case {
    name: String,
    /// file name -> source, every *.j2 including main.j2
    j2: BTreeMap<String, String>,
    /// <name>.j2 -> source of <name>.tera, the Tera version of that template
    tera: BTreeMap<String, String>,
    data: Json,
    settings: Json,
}

impl Case {
    fn load(dir: &Path) -> Case {
        let mut j2 = BTreeMap::new();
        let mut tera = BTreeMap::new();
        for entry in fs::read_dir(dir).expect("case directory") {
            let path = entry.expect("case directory entry").path();
            let (Some(stem), Some(ext)) = (path.file_stem(), path.extension()) else {
                continue;
            };
            let read = || fs::read_to_string(&path).expect("template file");
            match ext.to_str() {
                Some("j2") => {
                    j2.insert(format!("{}.j2", stem.to_string_lossy()), read());
                }
                Some("tera") => {
                    tera.insert(format!("{}.j2", stem.to_string_lossy()), read());
                }
                _ => {}
            }
        }
        let read_json = |name: &str| {
            fs::read_to_string(dir.join(name))
                .map(|s| serde_json::from_str(&s).expect("valid JSON"))
                .unwrap_or_else(|_| json!({}))
        };
        Case {
            name: dir.file_name().unwrap().to_string_lossy().into_owned(),
            j2,
            tera,
            data: read_json("data.json"),
            settings: read_json("settings.json"),
        }
    }

    fn setting(&self, key: &str) -> bool {
        self.settings.get(key).and_then(Json::as_bool).unwrap_or(false)
    }
}

/// Jinja2 drops one trailing newline of a template (keep_trailing_newline=False); Tera keeps
/// it, so it gets the source as Jinja2 sees it. Done once, outside the timed loop.
fn drop_trailing_newline(source: &str) -> &str {
    let source = source.strip_suffix('\n').unwrap_or(source);
    source.strip_suffix('\r').unwrap_or(source)
}

/// One engine on one case: load parses main.j2, render renders what prepare parsed
trait Runner {
    fn load(&mut self);
    fn render(&self) -> Result<String, String>;
}

/// Empty when the case uses no option outside `supported`, else why not
fn unsupported_settings(case: &Case, supported: &[&str]) -> Option<String> {
    let missing: Vec<&str> = ["trim_blocks", "lstrip_blocks", "autoescape", "wide"]
        .into_iter()
        .filter(|key| case.setting(key) && !supported.contains(key))
        .collect();
    (!missing.is_empty()).then(|| format!("no {}", missing.join(", ")))
}

struct MiniJinjaRunner {
    // Leaked so the compiled template can borrow it for the life of the process, as a
    // loaded jinja2::Template is rendered directly, with no lookup by name per render
    env: &'static minijinja::Environment<'static>,
    template: minijinja::Template<'static, 'static>,
    source: &'static str,
    data: minijinja::Value,
}

/// Writes JSON as Python's json.dumps does: ", " and ": " between items (or "," and newlines
/// with an indent) and \uXXXX for every non-ASCII character (ensure_ascii)
struct PythonJsonFormatter {
    indent: Option<Vec<u8>>,
    depth: usize,
    has_value: bool,
}

impl PythonJsonFormatter {
    fn newline<W: ?Sized + std::io::Write>(&self, w: &mut W) -> std::io::Result<()> {
        if let Some(indent) = &self.indent {
            w.write_all(b"\n")?;
            for _ in 0..self.depth {
                w.write_all(indent)?;
            }
        }
        Ok(())
    }
    fn begin<W: ?Sized + std::io::Write>(&mut self, w: &mut W, open: &[u8]) -> std::io::Result<()> {
        self.depth += 1;
        self.has_value = false;
        w.write_all(open)
    }
    fn end<W: ?Sized + std::io::Write>(&mut self, w: &mut W, close: &[u8]) -> std::io::Result<()> {
        self.depth -= 1;
        if self.has_value {
            self.newline(w)?;
        }
        self.has_value = true;
        w.write_all(close)
    }
    fn item<W: ?Sized + std::io::Write>(&mut self, w: &mut W, first: bool) -> std::io::Result<()> {
        if !first {
            w.write_all(if self.indent.is_some() { b"," } else { b", " })?;
        }
        self.has_value = true;
        self.newline(w)
    }
}

impl serde_json::ser::Formatter for PythonJsonFormatter {
    fn begin_array<W: ?Sized + std::io::Write>(&mut self, w: &mut W) -> std::io::Result<()> {
        self.begin(w, b"[")
    }
    fn end_array<W: ?Sized + std::io::Write>(&mut self, w: &mut W) -> std::io::Result<()> {
        self.end(w, b"]")
    }
    fn begin_array_value<W: ?Sized + std::io::Write>(&mut self, w: &mut W, first: bool) -> std::io::Result<()> {
        self.item(w, first)
    }
    fn begin_object<W: ?Sized + std::io::Write>(&mut self, w: &mut W) -> std::io::Result<()> {
        self.begin(w, b"{")
    }
    fn end_object<W: ?Sized + std::io::Write>(&mut self, w: &mut W) -> std::io::Result<()> {
        self.end(w, b"}")
    }
    fn begin_object_key<W: ?Sized + std::io::Write>(&mut self, w: &mut W, first: bool) -> std::io::Result<()> {
        self.item(w, first)
    }
    fn begin_object_value<W: ?Sized + std::io::Write>(&mut self, w: &mut W) -> std::io::Result<()> {
        w.write_all(b": ")
    }
    fn write_string_fragment<W: ?Sized + std::io::Write>(&mut self, w: &mut W, fragment: &str) -> std::io::Result<()> {
        let mut rest = fragment;
        while let Some(pos) = rest.find(|c: char| !c.is_ascii()) {
            w.write_all(&rest.as_bytes()[..pos])?;
            let c = rest[pos..].chars().next().unwrap();
            let mut units = [0u16; 2];
            for unit in c.encode_utf16(&mut units) {
                write!(w, "\\u{unit:04x}")?;
            }
            rest = &rest[pos + c.len_utf8()..];
        }
        w.write_all(rest.as_bytes())
    }
}

/// `tojson` as Python Jinja2 and Hugging Face chat templates print it. MiniJinja's own
/// writes compact JSON (no spaces after `,` and `:`), so chat templates rendered with it
/// differ from Jinja2's in every tool definition; Rust LLM servers that run these templates
/// on MiniJinja register a filter like this one. The same serde_json pass as the built-in.
fn python_tojson(
    value: &minijinja::Value,
    kwargs: minijinja::value::Kwargs,
) -> Result<minijinja::Value, minijinja::Error> {
    use serde::Serialize;
    let indent: Option<usize> = kwargs.get("indent")?;
    kwargs.assert_all_used()?;
    let mut out = Vec::new();
    let formatter = PythonJsonFormatter {
        indent: indent.map(|n| vec![b' '; n]),
        depth: 0,
        has_value: false,
    };
    value
        .serialize(&mut serde_json::Serializer::with_formatter(&mut out, formatter))
        .map_err(|e| minijinja::Error::new(minijinja::ErrorKind::InvalidOperation, e.to_string()))?;
    Ok(minijinja::Value::from_safe_string(String::from_utf8(out).unwrap()))
}

impl MiniJinjaRunner {
    fn prepare(case: &Case) -> Result<Box<dyn Runner>, String> {
        if let Some(why) = unsupported_settings(case, &["trim_blocks", "lstrip_blocks", "autoescape"]) {
            return Err(why);
        }
        let mut env = minijinja::Environment::new();
        env.set_trim_blocks(case.setting("trim_blocks"));
        env.set_lstrip_blocks(case.setting("lstrip_blocks"));
        // What an embedder running Jinja2 templates adds: the contrib filters (truncate,
        // wordcount, ...) and Python's str/dict/list methods
        minijinja_contrib::add_to_environment(&mut env);
        env.set_unknown_method_callback(minijinja_contrib::pycompat::unknown_method_callback);
        env.add_filter("tojson", python_tojson);
        let autoescape = case.setting("autoescape");
        env.set_auto_escape_callback(move |_| {
            if autoescape {
                minijinja::AutoEscape::Html
            } else {
                minijinja::AutoEscape::None
            }
        });
        for (name, source) in &case.j2 {
            env.add_template_owned(name.clone(), source.clone())
                .map_err(|e| e.to_string())?;
        }
        let env: &'static minijinja::Environment<'static> = Box::leak(Box::new(env));
        let source: &'static str = Box::leak(case.j2["main.j2"].clone().into_boxed_str());
        Ok(Box::new(MiniJinjaRunner {
            env,
            template: env.get_template("main.j2").map_err(|e| e.to_string())?,
            source,
            data: minijinja::Value::from_serialize(&case.data),
        }))
    }
}

impl Runner for MiniJinjaRunner {
    fn load(&mut self) {
        let _ = black_box(self.env.template_from_str(self.source));
    }
    fn render(&self) -> Result<String, String> {
        self.template.render(&self.data).map_err(|e| format!("{e:#}"))
    }
}

struct TeraRunner {
    tera: tera::Tera,
    source: String,
    context: tera::Context,
}

impl TeraRunner {
    fn prepare(case: &Case) -> Result<Box<dyn Runner>, String> {
        if let Some(why) = unsupported_settings(case, &["autoescape"]) {
            return Err(why);
        }
        let mut tera = tera::Tera::new();
        // Tera escapes by template name suffix; every template here ends in .j2
        tera.autoescape_on(if case.setting("autoescape") {
            vec![".j2"]
        } else {
            vec![]
        });
        let sources: BTreeMap<&str, &str> = case
            .j2
            .iter()
            .map(|(name, source)| {
                let source = case.tera.get(name).unwrap_or(source);
                (name.as_str(), drop_trailing_newline(source))
            })
            .collect();
        tera.add_raw_templates(sources.iter().map(|(name, source)| (*name, *source)))
            .map_err(|e| e.to_string())?;
        Ok(Box::new(TeraRunner {
            tera,
            source: sources["main.j2"].to_owned(),
            context: tera::Context::from_serialize(&case.data).map_err(|e| e.to_string())?,
        }))
    }
}

impl Runner for TeraRunner {
    // Tera has no public parse-only step: adding a template parses it and relinks the set
    // (inheritance chains, components), the work done before main.j2 can render
    fn load(&mut self) {
        let _ = black_box(self.tera.add_raw_template("main.j2", &self.source));
    }
    fn render(&self) -> Result<String, String> {
        self.tera.render("main.j2", &self.context).map_err(|e| {
            let mut msg = e.to_string();
            let mut source = std::error::Error::source(&e);
            while let Some(s) = source {
                msg += &format!(": {s}");
                source = s.source();
            }
            msg
        })
    }
}

struct Engine {
    name: &'static str,
    prepare: fn(&Case) -> Result<Box<dyn Runner>, String>,
}

const ENGINES: &[Engine] = &[
    Engine {
        name: "minijinja",
        prepare: MiniJinjaRunner::prepare,
    },
    Engine {
        name: "tera",
        prepare: TeraRunner::prepare,
    },
];

/// Prepares and renders once; a panic, an error or a refusal becomes the reason it cannot run
fn try_prepare(engine: &Engine, case: &Case) -> Result<(Box<dyn Runner>, String), String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let runner = (engine.prepare)(case)?;
        let output = runner.render()?;
        Ok((runner, output))
    }))
    .unwrap_or_else(|_| Err("panicked".to_owned()))
}

/// CPU time of this thread in ns, as Google Benchmark's cpu_time
fn thread_cpu_ns() -> f64 {
    let mut ts = libc::timespec { tv_sec: 0, tv_nsec: 0 };
    // SAFETY: ts is a valid timespec to write into
    unsafe { libc::clock_gettime(libc::CLOCK_THREAD_CPUTIME_ID, &mut ts) };
    ts.tv_sec as f64 * 1e9 + ts.tv_nsec as f64
}

/// Runs `f` `iterations` times; (wall ns, CPU ns) per iteration
fn time_batch(f: &mut dyn FnMut(), iterations: u64) -> (f64, f64) {
    let (wall, cpu) = (std::time::Instant::now(), thread_cpu_ns());
    for _ in 0..iterations {
        f();
    }
    let n = iterations as f64;
    (wall.elapsed().as_nanos() as f64 / n, (thread_cpu_ns() - cpu) / n)
}

/// Grows the batch until it runs for min_time, as Google Benchmark does; the iteration count
fn estimate_iterations(f: &mut dyn FnMut(), min_time: f64) -> u64 {
    let mut iterations = 1u64;
    loop {
        let (_, cpu) = time_batch(f, iterations);
        let elapsed = cpu * iterations as f64;
        if elapsed >= min_time * 1e9 || iterations >= 1 << 30 {
            return iterations;
        }
        let multiplier = if elapsed < min_time * 1e8 {
            10.0
        } else {
            min_time * 1e9 * 1.4 / elapsed.max(1.0)
        };
        iterations = ((iterations as f64 * multiplier).ceil() as u64).max(iterations + 1);
    }
}

struct Options {
    cases_dir: PathBuf,
    dump_dir: Option<PathBuf>,
    filter: Option<regex::Regex>,
    min_time: f64,
    repetitions: usize,
    out: Option<PathBuf>,
    aggregates_only: bool,
}

fn parse_args() -> Options {
    let mut options = Options {
        cases_dir: PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../cases"),
        dump_dir: None,
        filter: None,
        min_time: 0.5,
        repetitions: 1,
        out: None,
        aggregates_only: false,
    };
    for arg in std::env::args().skip(1) {
        let (flag, value) = arg.split_once('=').unwrap_or((arg.as_str(), "true"));
        match flag {
            "--cases-dir" => options.cases_dir = value.into(),
            "--dump-dir" => options.dump_dir = Some(value.into()),
            "--benchmark_filter" => options.filter = Some(regex::Regex::new(value).expect("valid --benchmark_filter")),
            "--benchmark_min_time" => {
                options.min_time = value
                    .trim_end_matches('s')
                    .parse()
                    .expect("--benchmark_min_time=<seconds>s")
            }
            "--benchmark_repetitions" => options.repetitions = value.parse().expect("--benchmark_repetitions=<n>"),
            "--benchmark_out" => options.out = Some(value.into()),
            "--benchmark_out_format" if value == "json" => {}
            "--benchmark_report_aggregates_only" => options.aggregates_only = value == "true",
            _ => {
                eprintln!("unrecognized argument: {arg}");
                std::process::exit(1);
            }
        }
    }
    options
}

fn median(values: &mut [f64]) -> f64 {
    values.sort_by(f64::total_cmp);
    let mid = values.len() / 2;
    if values.len() % 2 == 1 {
        values[mid]
    } else {
        (values[mid - 1] + values[mid]) / 2.0
    }
}

/// Google Benchmark JSON records for one benchmark: the runs, then the aggregates
fn measure(name: &str, f: &mut dyn FnMut(), options: &Options) -> Vec<Json> {
    let iterations = estimate_iterations(f, options.min_time);
    let runs: Vec<(f64, f64)> = (0..options.repetitions).map(|_| time_batch(f, iterations)).collect();
    let record = |run_type: &str, real: f64, cpu: f64| {
        json!({"name": name, "run_name": name, "run_type": run_type, "iterations": iterations,
               "real_time": real, "cpu_time": cpu, "time_unit": "ns"})
    };
    let mut records = Vec::new();
    if options.repetitions == 1 || !options.aggregates_only {
        records.extend(runs.iter().map(|&(real, cpu)| record("iteration", real, cpu)));
    }
    if options.repetitions > 1 {
        let n = runs.len() as f64;
        let mean = |pick: fn(&(f64, f64)) -> f64| runs.iter().map(pick).sum::<f64>() / n;
        let stddev = |pick: fn(&(f64, f64)) -> f64, m: f64| {
            (runs.iter().map(|r| (pick(r) - m).powi(2)).sum::<f64>() / (n - 1.0)).sqrt()
        };
        let (real_mean, cpu_mean) = (mean(|r| r.0), mean(|r| r.1));
        let (real_sd, cpu_sd) = (stddev(|r| r.0, real_mean), stddev(|r| r.1, cpu_mean));
        let mut reals: Vec<f64> = runs.iter().map(|r| r.0).collect();
        let mut cpus: Vec<f64> = runs.iter().map(|r| r.1).collect();
        for (aggregate, real, cpu) in [
            ("mean", real_mean, cpu_mean),
            ("median", median(&mut reals), median(&mut cpus)),
            ("stddev", real_sd, cpu_sd),
            ("cv", real_sd / real_mean, cpu_sd / cpu_mean),
        ] {
            let mut r = record("aggregate", real, cpu);
            r["name"] = json!(format!("{name}_{aggregate}"));
            r["aggregate_name"] = json!(aggregate);
            records.push(r);
        }
    }
    let shown = records
        .iter()
        .rev()
        .find(|r| r["aggregate_name"] == "median")
        .unwrap_or(&records[0]);
    eprintln!("{name:45} {:14.0} ns", shown["cpu_time"].as_f64().unwrap());
    records
}

/// Versions of the engines as Cargo.lock pins them
fn engine_versions() -> Json {
    let lock = include_str!("../Cargo.lock");
    let mut versions = serde_json::Map::new();
    for engine in ENGINES {
        let header = format!("name = \"{}\"\nversion = \"", engine.name);
        if let Some(start) = lock.find(&header).map(|i| i + header.len()) {
            let version = &lock[start..start + lock[start..].find('"').unwrap()];
            versions.insert(format!("{}_version", engine.name), json!(version));
        }
    }
    Json::Object(versions)
}

fn main() {
    let options = parse_args();
    let mut dirs: Vec<PathBuf> = fs::read_dir(&options.cases_dir)
        .expect("--cases-dir")
        .map(|e| e.expect("cases directory entry").path())
        .filter(|p| p.join("main.j2").exists())
        .collect();
    dirs.sort();
    let cases: Vec<Case> = dirs.iter().map(|d| Case::load(d)).collect();
    // A failed case is reported, not printed by the default panic hook
    std::panic::set_hook(Box::new(|_| {}));

    let mut benchmarks = Vec::new();
    for engine in ENGINES {
        for case in &cases {
            let prepared = try_prepare(engine, case);
            if let Some(dump_dir) = &options.dump_dir {
                let dir = dump_dir.join(engine.name);
                fs::create_dir_all(&dir).expect("--dump-dir");
                let (file, text) = match &prepared {
                    Ok((_, output)) => (format!("{}.txt", case.name), output),
                    Err(error) => (format!("{}.err", case.name), error),
                };
                fs::write(dir.join(file), text).expect("--dump-dir");
                continue;
            }
            let Ok((mut runner, _)) = prepared else { continue };
            let selected = |kind: &str| {
                let name = format!("{}/{kind}/{}", engine.name, case.name);
                options
                    .filter
                    .as_ref()
                    .is_none_or(|f| f.is_match(&name))
                    .then_some(name)
            };
            if let Some(name) = selected("Load") {
                benchmarks.extend(measure(&name, &mut || runner.load(), &options));
            }
            if let Some(name) = selected("Render") {
                benchmarks.extend(measure(&name, &mut || drop(black_box(runner.render())), &options));
            }
        }
    }
    if options.dump_dir.is_some() {
        return;
    }

    let mut context = json!({"engine": "rust", "library_build_type": "release",
                             "host_name": fs::read_to_string("/proc/sys/kernel/hostname").unwrap_or_default().trim()});
    context
        .as_object_mut()
        .unwrap()
        .extend(engine_versions().as_object().unwrap().clone());
    let report = serde_json::to_string_pretty(&json!({"context": context, "benchmarks": benchmarks})).unwrap();
    match &options.out {
        Some(path) => fs::write(path, report).expect("--benchmark_out"),
        None => println!("{report}"),
    }
}
