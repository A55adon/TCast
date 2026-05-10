fn main() {
    #[cfg(target_os = "windows")]
    {
        let mut resource = winresource::WindowsResource::new();
        resource.set_icon("../assets/t-cast-favicon.ico");
        resource.set("FileDescription", "TCast");
        resource.set("ProductName", "TCast");
        resource.set("CompanyName", "A55adon");
        resource.set("OriginalFilename", "tcast-rust.exe");
        if let Err(error) = resource.compile() {
            println!("cargo:warning=Could not embed Windows icon resource: {error}");
        }
    }
}
