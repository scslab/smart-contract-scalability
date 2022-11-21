
use std::env;
use std::fs::File;
use std::io::prelude::*;

extern crate inject_metering;

fn main() {
    let args: Vec<String> = env::args().collect();
    println!("{:?}", args);

    if args.len() != 3 {
    	println!("invalid arg count");
    	return;
    }

    let wasm_file = &args[1];

    let mut f = File::open(wasm_file).expect("should be able to read file");

    let mut buffer = Vec::new();
    f.read_to_end(&mut buffer).expect("read to end");

    let out_mod = match inject_metering::add_metering(&buffer) {
    	Some(m) => m,
    	None => {println!("metering failed"); return; },
    };

    let out_filename = &args[2];

    let mut out_f = File::create(out_filename).expect("opened");
    out_f.write_all(&out_mod).ok();


}