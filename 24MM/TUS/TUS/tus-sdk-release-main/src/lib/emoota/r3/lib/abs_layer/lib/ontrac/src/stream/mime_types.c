/****************************************************************************
 *
 *         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 *
 *  The software and information contained herein is the proprietary and
 *  confidential property of Airbiquity Inc. and shall not be disclosed
 *  in whole or in part. Possession, use, reproduction or transfer of
 *  this software and information is prohibited without the express written
 *  permission of Airbiquity Inc.
 *
 *  Copyright (c) 2019 Airbiquity Inc.  All rights reserved.
 *
 ****************************************************************************/
/**
 * @file mime_types.c
 * @date Sep 16, 2019
 * @author mvogel
 * @copyright Airbiquity, Inc.
 *
 * @brief {Help Me}
 */

#include <ontrac/stream/abq_io.h>
#include <ontrac/unicode/utf8_utils.h>


struct mime_type_by_ext {
	const cstr_t ext;
	// this is a pointer for no other reason then to circumvent
	//  the stupid "error: initializer element is not constant"
	const cstr_t mime;
};

cstr_t http_headers_find_content_type(cstr_t file_name, cstr_t if_none_found) {
    static const struct mime_type_by_ext mimes_by_ext[] = {
            {".exe",             // parasoft-suppress CERT_C-MSC41-a-1 "c0371. This string does not contain sensitive information."
             MISC_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0372. This string does not contain sensitive information."
            {".json",            // parasoft-suppress CERT_C-MSC41-a-1 "c0373. This string does not contain sensitive information."
             JSON_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0374. This string does not contain sensitive information."
            {".js",              // parasoft-suppress CERT_C-MSC41-a-1 "c0375. This string does not contain sensitive information."
             JS_CONTENT_TYPE},   // parasoft-suppress CERT_C-MSC41-a-1 "c0376. This string does not contain sensitive information."
            {".gz",              // parasoft-suppress CERT_C-MSC41-a-1 "c0377. This string does not contain sensitive information."
             GZ_CONTENT_TYPE},   // parasoft-suppress CERT_C-MSC41-a-1 "c0378. This string does not contain sensitive information."
            {".tar",             // parasoft-suppress CERT_C-MSC41-a-1 "c0379. This string does not contain sensitive information."
             TAR_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0380. This string does not contain sensitive information."
            {".tgz",             // parasoft-suppress CERT_C-MSC41-a-1 "c0381. This string does not contain sensitive information."
             TGZ_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0382. This string does not contain sensitive information."
            {".xhtml",           // parasoft-suppress CERT_C-MSC41-a-1 "c0383. This string does not contain sensitive information."
             XHTML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0384. This string does not contain sensitive information."
            {".pdf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0385. This string does not contain sensitive information."
             PDF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0386. This string does not contain sensitive information."
            {".rtf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0387. This string does not contain sensitive information."
             RTF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0388. This string does not contain sensitive information."
            {".xsl",             // parasoft-suppress CERT_C-MSC41-a-1 "c0389. This string does not contain sensitive information."
             XML_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0390. This string does not contain sensitive information."
            {".xslt",            // parasoft-suppress CERT_C-MSC41-a-1 "c0391. This string does not contain sensitive information."
             XML_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0392. This string does not contain sensitive information."
            {".eps",             // parasoft-suppress CERT_C-MSC41-a-1 "c0393. This string does not contain sensitive information."
             PS_CONTENT_TYPE},   // parasoft-suppress CERT_C-MSC41-a-1 "c0394. This string does not contain sensitive information."
            {".ps",              // parasoft-suppress CERT_C-MSC41-a-1 "c0395. This string does not contain sensitive information."
             PS_CONTENT_TYPE},   // parasoft-suppress CERT_C-MSC41-a-1 "c0396. This string does not contain sensitive information."
            {".swf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0397. This string does not contain sensitive information."
             SWF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0398. This string does not contain sensitive information."
            {".arj",             // parasoft-suppress CERT_C-MSC41-a-1 "c0399. This string does not contain sensitive information."
             ARG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0400. This string does not contain sensitive information."
            {".rar",             // parasoft-suppress CERT_C-MSC41-a-1 "c0401. This string does not contain sensitive information."
             ARG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0402. This string does not contain sensitive information."
            {".zip",             // parasoft-suppress CERT_C-MSC41-a-1 "c0403. This string does not contain sensitive information."
             ZIP_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0404. This string does not contain sensitive information."
            {".ppt",             // parasoft-suppress CERT_C-MSC41-a-1 "c0405. This string does not contain sensitive information."
             PPT_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0406. This string does not contain sensitive information."
            {".doc",             // parasoft-suppress CERT_C-MSC41-a-1 "c0407. This string does not contain sensitive information."
             DOC_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0408. This string does not contain sensitive information."
            {".xls",             // parasoft-suppress CERT_C-MSC41-a-1 "c0409. This string does not contain sensitive information."
             XLS_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0410. This string does not contain sensitive information."
            {".torrent",         // parasoft-suppress CERT_C-MSC41-a-1 "c0411. This string does not contain sensitive information."
             TORNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0412. This string does not contain sensitive information."
            {".ttf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0413. This string does not contain sensitive information."
             SFNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0414. This string does not contain sensitive information."
            {".cff",             // parasoft-suppress CERT_C-MSC41-a-1 "c0415. This string does not contain sensitive information."
             SFNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0416. This string does not contain sensitive information."
            {".otf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0417. This string does not contain sensitive information."
             SFNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0418. This string does not contain sensitive information."
            {".aat",             // parasoft-suppress CERT_C-MSC41-a-1 "c0419. This string does not contain sensitive information."
             SFNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0420. This string does not contain sensitive information."
            {".sil",             // parasoft-suppress CERT_C-MSC41-a-1 "c0421. This string does not contain sensitive information."
             SFNT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0422. This string does not contain sensitive information."
            {".pfr",             // parasoft-suppress CERT_C-MSC41-a-1 "c0423. This string does not contain sensitive information."
             PRF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0424. This string does not contain sensitive information."
            {".woff",            // parasoft-suppress CERT_C-MSC41-a-1 "c0425. This string does not contain sensitive information."
             WOFF_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0426. This string does not contain sensitive information."
            {".html",            // parasoft-suppress CERT_C-MSC41-a-1 "c0427. This string does not contain sensitive information."
             HTML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0428. This string does not contain sensitive information."
            {".htm",             // parasoft-suppress CERT_C-MSC41-a-1 "c0429. This string does not contain sensitive information."
             HTML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0430. This string does not contain sensitive information."
            {".txt",             // parasoft-suppress CERT_C-MSC41-a-1 "c0431. This string does not contain sensitive information."
             TEXT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0432. This string does not contain sensitive information."
            {".css",             // parasoft-suppress CERT_C-MSC41-a-1 "c0433. This string does not contain sensitive information."
             CSS_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0434. This string does not contain sensitive information."
            {".xml",             // parasoft-suppress CERT_C-MSC41-a-1 "c0435. This string does not contain sensitive information."
             XML_CONTENT_TYPE2}, // parasoft-suppress CERT_C-MSC41-a-1 "c0436. This string does not contain sensitive information."
            {".csv",             // parasoft-suppress CERT_C-MSC41-a-1 "c0437. This string does not contain sensitive information."
             CSV_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0438. This string does not contain sensitive information."
            {".sgm",             // parasoft-suppress CERT_C-MSC41-a-1 "c0439. This string does not contain sensitive information."
             SGML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0440. This string does not contain sensitive information."
            {".shtm",            // parasoft-suppress CERT_C-MSC41-a-1 "c0441. This string does not contain sensitive information."
             HTML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0442. This string does not contain sensitive information."
            {".shtml",           // parasoft-suppress CERT_C-MSC41-a-1 "c0443. This string does not contain sensitive information."
             HTML_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0444. This string does not contain sensitive information."
            {".gif",             // parasoft-suppress CERT_C-MSC41-a-1 "c0445. This string does not contain sensitive information."
             GIF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0446. This string does not contain sensitive information."
            {".ief",             // parasoft-suppress CERT_C-MSC41-a-1 "c0447. This string does not contain sensitive information."
             IEF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0448. This string does not contain sensitive information."
            {".jpeg",            // parasoft-suppress CERT_C-MSC41-a-1 "c0449. This string does not contain sensitive information."
             JPEG_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0450. This string does not contain sensitive information."
            {".jpg",             // parasoft-suppress CERT_C-MSC41-a-1 "c0451. This string does not contain sensitive information."
             JPEG_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0452. This string does not contain sensitive information."
            {".jpm",             // parasoft-suppress CERT_C-MSC41-a-1 "c0453. This string does not contain sensitive information."
             JPM_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0454. This string does not contain sensitive information."
            {".jpx",             // parasoft-suppress CERT_C-MSC41-a-1 "c0455. This string does not contain sensitive information."
             JPX_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0456. This string does not contain sensitive information."
            {".png",             // parasoft-suppress CERT_C-MSC41-a-1 "c0457. This string does not contain sensitive information."
             PNG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0458. This string does not contain sensitive information."
            {".svg",             // parasoft-suppress CERT_C-MSC41-a-1 "c0459. This string does not contain sensitive information."
             SVG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0460. This string does not contain sensitive information."
            {".tif",             // parasoft-suppress CERT_C-MSC41-a-1 "c0461. This string does not contain sensitive information."
             TIFF_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0462. This string does not contain sensitive information."
            {".tiff",            // parasoft-suppress CERT_C-MSC41-a-1 "c0463. This string does not contain sensitive information."
             TIFF_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0464. This string does not contain sensitive information."
            {".bmp",             // parasoft-suppress CERT_C-MSC41-a-1 "c0465. This string does not contain sensitive information."
             BMP_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0466. This string does not contain sensitive information."
            {".ico",             // parasoft-suppress CERT_C-MSC41-a-1 "c0467. This string does not contain sensitive information."
             ICO_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0468. This string does not contain sensitive information."
            {".pct",             // parasoft-suppress CERT_C-MSC41-a-1 "c0469. This string does not contain sensitive information."
             PCT_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0470. This string does not contain sensitive information."
            {".pict",            // parasoft-suppress CERT_C-MSC41-a-1 "c0471. This string does not contain sensitive information."
             PICT_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0472. This string does not contain sensitive information."
            {".rgb",             // parasoft-suppress CERT_C-MSC41-a-1 "c0473. This string does not contain sensitive information."
             RGB_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0474. This string does not contain sensitive information."
            {".mp3",             // parasoft-suppress CERT_C-MSC41-a-1 "c0475. This string does not contain sensitive information."
             MP3_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0476. This string does not contain sensitive information."
            {".oga",             // parasoft-suppress CERT_C-MSC41-a-1 "c0477. This string does not contain sensitive information."
             OGG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0478. This string does not contain sensitive information."
            {".ogg",             // parasoft-suppress CERT_C-MSC41-a-1 "c0479. This string does not contain sensitive information."
             OGG_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0480. This string does not contain sensitive information."
            {".aac",             // parasoft-suppress CERT_C-MSC41-a-1 "c0481. This string does not contain sensitive information."
             AAC_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0482. This string does not contain sensitive information."
            {".aif",             // parasoft-suppress CERT_C-MSC41-a-1 "c0483. This string does not contain sensitive information."
             AIF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0484. This string does not contain sensitive information."
            {".m3u",             // parasoft-suppress CERT_C-MSC41-a-1 "c0485. This string does not contain sensitive information."
             M3U_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0486. This string does not contain sensitive information."
            {".mid",             // parasoft-suppress CERT_C-MSC41-a-1 "c0487. This string does not contain sensitive information."
             MIDI_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0488. This string does not contain sensitive information."
            {".ra",              // parasoft-suppress CERT_C-MSC41-a-1 "c0489. This string does not contain sensitive information."
             RAM_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0490. This string does not contain sensitive information."
            {".ram",             // parasoft-suppress CERT_C-MSC41-a-1 "c0491. This string does not contain sensitive information."
             RAM_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0492. This string does not contain sensitive information."
            {".wav",             // parasoft-suppress CERT_C-MSC41-a-1 "c0493. This string does not contain sensitive information."
             WAV_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0494. This string does not contain sensitive information."
            {".mpeg",            // parasoft-suppress CERT_C-MSC41-a-1 "c0495. This string does not contain sensitive information."
             MPEG_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0496. This string does not contain sensitive information."
            {".mpg",             // parasoft-suppress CERT_C-MSC41-a-1 "c0497. This string does not contain sensitive information."
             MPEG_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0498. This string does not contain sensitive information."
            {".avi",             // parasoft-suppress CERT_C-MSC41-a-1 "c0499. This string does not contain sensitive information."
             AVI_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0500. This string does not contain sensitive information."
            {".mov",             // parasoft-suppress CERT_C-MSC41-a-1 "c0501. This string does not contain sensitive information."
             MOV_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0502. This string does not contain sensitive information."
            {".mp4",             // parasoft-suppress CERT_C-MSC41-a-1 "c0503. This string does not contain sensitive information."
             MP4_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0504. This string does not contain sensitive information."
            {".ogv",             // parasoft-suppress CERT_C-MSC41-a-1 "c0505. This string does not contain sensitive information."
             OGV_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0506. This string does not contain sensitive information."
            {".qt",              // parasoft-suppress CERT_C-MSC41-a-1 "c0507. This string does not contain sensitive information."
             MOV_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0508. This string does not contain sensitive information."
            {".webm",            // parasoft-suppress CERT_C-MSC41-a-1 "c0509. This string does not contain sensitive information."
             WEBM_CONTENT_TYPE}, // parasoft-suppress CERT_C-MSC41-a-1 "c0510. This string does not contain sensitive information."
            {".asf",             // parasoft-suppress CERT_C-MSC41-a-1 "c0511. This string does not contain sensitive information."
             ASF_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0512. This string does not contain sensitive information."
            {".m4v",             // parasoft-suppress CERT_C-MSC41-a-1 "c0513. This string does not contain sensitive information."
             M4V_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0514. This string does not contain sensitive information."
            {".wrl",             // parasoft-suppress CERT_C-MSC41-a-1 "c0515. This string does not contain sensitive information."
             WRL_CONTENT_TYPE},  // parasoft-suppress CERT_C-MSC41-a-1 "c0516. This string does not contain sensitive information."
            {NULL,   NULL}// null terminator
    };
    cstr_t rvalue = if_none_found;
    cstr_t file_ext = file_name;
    int32_t file_ext_len = utf8_byte_length(file_ext, -1);
    int32_t delim_len = utf8_codec.bytesize('.');
    while (file_ext_len >= delim_len) {
        int32_t start_of_ext = utf8_index_of_char(&file_ext[delim_len], file_ext_len, '.');
        if (0 > start_of_ext) {
            break;
        }
        start_of_ext += delim_len;
        file_ext = &file_ext[start_of_ext];
        file_ext_len -= start_of_ext;
    }
    if (utf8_starts_with(file_ext, -1, ".", delim_len)) { // parasoft-suppress CERT_C-MSC41-a-1 "c0517. This string does not contain sensitive information."
        // Now we should have everything after the final decimal point (inclusive)
        for (int32_t index=0; mimes_by_ext[index].ext != NULL; index++) {
            cstr_t mime_ext = mimes_by_ext[index].ext;
            if (0 == utf8_compare_insensitive(mime_ext, file_ext, -1)) {
                // found a matching extension, set rvalue to match and break out of here
                rvalue = mimes_by_ext[index].mime;
                break;
            }
        }
    } else {
        // file has no extension,
        // nothing we can do short of internal file inspection of magic numbers.
        // so we will just return the if_none_found value for now.
    }
    return rvalue;
}
