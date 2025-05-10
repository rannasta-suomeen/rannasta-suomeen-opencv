
use std::{error::Error, ffi::CString};

use super::*;



#[tokio::test]
async fn test_main() -> Result<(), Box<dyn Error>> {
    let url_list = [
        "https://viinarannasta.ee/images/media/2021/09/I9myz03308.jpg",
        "https://viinarannasta.ee/images/media/2024/11/AK8fF05510.webp"
    ];

    for (i, url) in url_list.iter().enumerate() {
        let options = Options { url, tmp_dir: PathBuf::from("/tmp"), id: i, debug: url_list.len() < 2, verbose: true };
        let id = options.id;
        remove_background_url(options, |out| async move {
            tokio::fs::copy(out, format!("out/{}.png", id)).await?;
            Ok(())
        }).await?;
    }


    Ok(())
}