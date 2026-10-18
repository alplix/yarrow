//! Builds the bundled data files from the public sources:
//!
//! ```text
//! cargo run -p yarrow-share --example datagen -- cities15000.txt ne_110m_land.geojson
//! ```
//!
//! * `cities15000.txt` is the GeoNames dump (CC BY 4.0, https://www.geonames.org). Cities of at least 30 000 people
//!   are kept as `country<TAB>name<TAB>lat<TAB>lon<TAB>population`, coordinates rounded to two decimals.
//! * `ne_110m_land.geojson` is Natural Earth (public domain). The land outline is written as compact integer
//!   polygons (tenths of a degree) for the map in the app.

use std::fs;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let cities = fs::read_to_string(&args[1]).expect("cities file");
    let mut rows: Vec<(String, String, f64, f64, u64)> = Vec::new();
    for l in cities.lines() {
        let f: Vec<&str> = l.split('\t').collect();
        if f.len() < 15 {
            continue;
        }
        let pop: u64 = f[14].parse().unwrap_or(0);
        if pop < 30_000 {
            continue;
        }
        rows.push((f[8].to_string(), f[1].to_string(), f[4].parse().unwrap(), f[5].parse().unwrap(), pop));
    }
    rows.sort_by(|a, b| (&a.0, &a.1).cmp(&(&b.0, &b.1)));
    rows.dedup_by(|a, b| a.0 == b.0 && a.1 == b.1);
    let mut out = String::new();
    for r in &rows {
        out.push_str(&format!("{}\t{}\t{:.2}\t{:.2}\t{}\n", r.0, r.1, r.2, r.3, r.4));
    }
    fs::write("crates/yarrow-share/data/cities.tsv", &out).unwrap();
    println!("{} cities, {} bytes", rows.len(), out.len());

    let land: serde_json::Value = serde_json::from_str(&fs::read_to_string(&args[2]).expect("land file")).unwrap();
    let mut polys: Vec<Vec<[i32; 2]>> = Vec::new();
    for f in land["features"].as_array().unwrap() {
        let g = &f["geometry"];
        let rings: Vec<&serde_json::Value> = match g["type"].as_str().unwrap() {
            "Polygon" => vec![&g["coordinates"][0]],
            "MultiPolygon" => g["coordinates"].as_array().unwrap().iter().map(|p| &p[0]).collect(),
            _ => vec![],
        };
        for ring in rings {
            let mut pts: Vec<[i32; 2]> = Vec::new();
            for p in ring.as_array().unwrap() {
                let q = [(p[0].as_f64().unwrap() * 10.0).round() as i32, (p[1].as_f64().unwrap() * 10.0).round() as i32];
                if pts.last() != Some(&q) {
                    pts.push(q);
                }
            }
            polys.push(pts);
        }
    }
    let json = serde_json::to_string(&polys).unwrap();
    fs::create_dir_all("app/ui/public").unwrap();
    fs::write("app/ui/public/land.json", &json).unwrap();
    println!("{} land polygons, {} bytes", polys.len(), json.len());
}
