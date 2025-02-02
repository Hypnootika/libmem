use std::env;

#[cfg(feature = "fetch")]
/// Downloads and adds the libmem path as a search path for linking
// NOTE: This always fetches a static library
fn download_and_resolve_libmem() {
    use flate2::read::GzDecoder;
    use reqwest::StatusCode;
    use std::{
        fs::File,
        io::{self, Cursor},
        path::PathBuf,
    };
    use tar::Archive;

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    // Get download URL
    let version = env::var("CARGO_PKG_VERSION").unwrap();
    let os_name = env::consts::OS;
    let arch = env::consts::ARCH;
    let target_env = if cfg!(target_os = "linux") && cfg!(feature = "static") {
        // Always use musl for static linking on Linux
        "musl".to_owned()
    } else {
        env::var("CARGO_CFG_TARGET_ENV").unwrap()
    };
    let build_type = if cfg!(target_os = "windows") {
        "static-mt"
    } else {
        "static"
    };
    let fullname = format!(
        "libmem-{}-{}-{}-{}-{}",
        version, arch, os_name, target_env, build_type
    );
    let archive_ext = "tar.gz";
    let archive = format!("{}.{}", fullname, archive_ext);
    let download_url = format!(
        "https://github.com/rdbo/libmem/releases/download/{}/{}",
        version, archive
    );
    eprintln!("Download URL: {}", download_url);

    // Download archive if necessary
    let archive_path = out_dir.join(archive);
    if !archive_path.exists() {
        let req = reqwest::blocking::get(&download_url).expect(&format!(
            "Failed to download libmem archive from: {}",
            download_url
        ));

        if !StatusCode::is_success(&req.status()) {
            panic!(
                "Request to download URL failed with code '{}': {}",
                req.status(),
                download_url
            );
        }

        let content = req.bytes().expect("Failed to get download content");
        eprintln!("Content size: {} bytes", content.len());

        let mut file = File::create(&archive_path).expect(&format!(
            "Failed to create libmem archive on path: {:?}",
            archive_path
        ));
        io::copy(&mut Cursor::new(content), &mut file)
            .expect("Failed to copy downloaded content to archive");
    }

    // Extract archive if necessary
    let archive_dir = out_dir.join(fullname);
    if !archive_dir.exists() {
        let tar_gz = File::open(&archive_path)
            .expect(&format!("Failed to open archive file: {:?}", archive_path));
        let tar = GzDecoder::new(tar_gz);
        let mut tar_archive = Archive::new(tar);
        tar_archive
            .unpack(out_dir)
            .expect("Failed to extract libmem archive");
    }

    // Properly add library path for linking
    let search_path = if cfg!(target_os = "windows") {
        archive_dir.join("lib").join("release")
    } else {
        archive_dir.join("lib")
    };

    eprintln!("Search path: {}", search_path.display());
    println!("cargo:rustc-link-search={}", search_path.display());
}

fn main() {
    if let Ok(path) = env::var("LIBMEM_DIR") {
        println!("cargo:rustc-link-search={}", path);
    } else {
        #[cfg(feature = "fetch")]
        download_and_resolve_libmem();
    }

    // Resolve link dependencies
    let deps = if cfg!(target_os = "windows") {
        vec!["user32", "psapi", "ntdll"]
    } else if cfg!(target_os = "linux") {
        vec!["dl", "m", "stdc++"]
    } else if cfg!(target_os = "freebsd") {
        vec!["dl", "kvm", "procstat", "elf", "m", "stdc++"]
    } else {
        vec![]
    };

    for dep in deps {
        println!("cargo:rustc-link-lib={}", dep);
    }
    println!("cargo:rustc-link-lib=libmem");
}
