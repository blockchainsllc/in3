package in3;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * a simple Storage Provider storing the cache in the temp-folder.
 */
public class TempStorageProvider implements StorageProvider {

    private static File tmp = new File(System.getProperty("java.io.tmpdir"));
    private static String in3Prefix = "in3_cache_";

    @Override
    public byte[] getItem(String key) {

        File f = new File(tmp, in3Prefix + key);
        if (f.exists()) {
            BufferedInputStream is = null;
            try {
                is = new BufferedInputStream(new FileInputStream(f));
                byte[] content = new byte[(int) f.length()];
                int offset = 0;
                while (offset < content.length)
                    offset += is.read(content, offset, content.length - offset);
                return content;
            } catch (Exception ex) {
                return null;
            } finally {
                try {
                    if (is != null)
                        is.close();
                } catch (IOException io) {
                }
            }
        }
        return null;
    }

    @Override
    public void setItem(String key, byte[] content) {
        try {
            FileOutputStream os = new FileOutputStream(new File(tmp, in3Prefix + key));
            os.write(content);
            os.close();
        } catch (IOException ex) {
        }
    }

}