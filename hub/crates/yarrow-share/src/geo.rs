//! The bundled offline city list (GeoNames, CC BY 4.0, cities of 30 000 people or more). The Hub only ever sends
//! the country code and the city name; the coordinates stay on this computer and are used to draw the map.

use serde::Serialize;
use std::sync::OnceLock;

const DATA: &str = include_str!("../data/cities.tsv");

#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct City {
    pub country: String,
    pub name: String,
    pub lat: f64,
    pub lon: f64,
    pub population: u64,
}

fn all() -> &'static [City] {
    static CITIES: OnceLock<Vec<City>> = OnceLock::new();
    CITIES.get_or_init(|| {
        DATA.lines()
            .filter_map(|l| {
                let f: Vec<&str> = l.split('\t').collect();
                if f.len() != 5 {
                    return None;
                }
                Some(City {
                    country: f[0].to_string(),
                    name: f[1].to_string(),
                    lat: f[2].parse().ok()?,
                    lon: f[3].parse().ok()?,
                    population: f[4].parse().ok()?,
                })
            })
            .collect()
    })
}

/// Country codes that have at least one city in the list.
pub fn countries() -> Vec<String> {
    let mut v: Vec<String> = all().iter().map(|c| c.country.clone()).collect();
    v.dedup();
    v
}

/// Cities of a country whose name contains `query` (case-insensitive), the biggest first.
pub fn cities(country: &str, query: &str, limit: usize) -> Vec<City> {
    let q = query.trim().to_lowercase();
    let mut v: Vec<City> = all()
        .iter()
        .filter(|c| c.country == country && (q.is_empty() || c.name.to_lowercase().contains(&q)))
        .cloned()
        .collect();
    v.sort_by(|a, b| b.population.cmp(&a.population).then(a.name.cmp(&b.name)));
    v.truncate(limit);
    v
}

/// The exact city, or `None` when it is not in the list.
pub fn find(country: &str, name: &str) -> Option<City> {
    all().iter().find(|c| c.country == country && c.name == name).cloned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_list_is_there() {
        assert!(all().len() > 15_000);
        assert!(countries().contains(&"TR".to_string()));
        let d = find("TR", "Denizli").unwrap();
        assert!((d.lat - 37.77).abs() < 0.1);
        assert!(find("TR", "Nowhere").is_none());
    }

    #[test]
    fn search_is_case_insensitive_and_biggest_first() {
        let v = cities("TR", "ANK", 5);
        assert_eq!(v[0].name, "Ankara");
        let all_tr = cities("TR", "", 1000);
        assert!(all_tr.windows(2).all(|w| w[0].population >= w[1].population));
        assert!(cities("XX", "", 10).is_empty());
    }
}
