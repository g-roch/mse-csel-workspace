// Import necessary modules
// - File and OpenOptions for file handling
use std::fs::{File, OpenOptions};
// - Read and Write traits for reading and writing to files
use std::io::{Read, Write};

// HELPER FUNCTIONS //
fn write_to_file(file_path: &str, content: &str) -> std::io::Result<()> {
    // Create or open the file for writing
    let mut file = OpenOptions::new()
        .write(true)
        .create(true)
        .open(file_path)?;

    // Write the content to the file
    file.write_all(content.as_bytes())?;
    Ok(())
}

fn read_from_file(file_path: &str) -> std::io::Result<String> {
    // Open the file for reading
    let mut file = File::open(file_path)?;
    let mut content = String::new();

    // Read the content of the file into a string
    file.read_to_string(&mut content)?;
    Ok(content)
}
// ---------------- //

// Define the main function
fn main() {
    // Define the device paths (these should match the ones created by your kernel module)
    let devices = vec!["/dev/mymodule", "/dev/mymodule.1", "/dev/mymodule.2"];
    // Define the content to write to each device
    let contents = vec![
        "Hello device 0 from Rust!",
        "Hello device 1 from Rust!",
        "Hello device 2 from Rust!",
    ];

    // Loop through each device and perform write and read operations
    for (i, device) in devices.iter().enumerate() {
        // Write to the device
        match write_to_file(device, contents[i]) {
            Ok(_) => println!("Content successfully written to {}", device),
            Err(e) => eprintln!("Failed to write to {}: {}", device, e),
        }

        // Read from the device
        match read_from_file(device) {
            Ok(content) => println!("Content read from {}: {}", device, content),
            Err(e) => eprintln!("Failed to read from {}: {}", device, e),
        }
    }
}
