use std::{error::Error, ffi::CString, future::Future, path::PathBuf};

use tokio::io::AsyncWriteExt;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));


mod tests;



pub struct Options<'options> {
    pub url: &'options str,
    pub tmp_dir: PathBuf,
    pub id: usize,
    pub debug: bool,
    pub verbose: bool
}

pub async fn remove_background_url<F, Fut>(options: Options<'_>, callback: F) -> Result<(), Box<dyn Error>> 
where
    F: Fn(PathBuf) -> Fut,
    Fut: Future<Output = Result<(), Box<dyn Error>>>
{
    let tmp_input_file_path = PathBuf::from(format!("tmp-input-{}", options.id));
    let tmp_output_file_path = PathBuf::from(format!("tmp-output-{}.png", options.id));

    let full_input_path = options.tmp_dir.join(tmp_input_file_path);
    let full_output_path = options.tmp_dir.join(tmp_output_file_path);

    let mut tmp_file = tokio::fs::File::create(&full_input_path).await?;

    let mut stream = reqwest::get(options.url).await?;
    while let Some(chunk) = stream.chunk().await? {
        tmp_file.write_all(&chunk).await?;
    }

    let _full_input_path = full_input_path.as_os_str().to_str().ok_or(format!("Invalid path: {:#?}", &full_input_path))?;
    let _full_output_path = full_output_path.as_os_str().to_str().ok_or(format!("Invalid path: {:#?}", &full_output_path))?;

    let input_path = CString::new(_full_input_path)?;
    let input_path = input_path.as_ptr();

    let output_path = CString::new(_full_output_path)?;
    let output_path = output_path.as_ptr();


    let _options = ResolveOptions { debug: options.debug, verbose: options.verbose, input_path, output_path };
    unsafe { run_image_tool(_options) };

    callback(full_output_path.clone()).await?;

    tokio::fs::remove_file(full_input_path).await?;
    tokio::fs::remove_file(full_output_path).await?;

    Ok(())
}


