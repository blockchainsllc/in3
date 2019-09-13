package in3;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.*;
import java.security.MessageDigest;
import java.util.Arrays;

public class Loader {

    private static boolean loaded = false;

    private static String getLibName() {
        final String os = System.getProperty("os.name").toLowerCase();
        final String arch = System.getProperty("os.arch").toLowerCase();
        final String model = System.getProperty("sun.arch.data.model");
        if (os.indexOf("linux") >= 0) {
            if (arch.indexOf("arm") >= 0)
                return "in3_jni_arm";
        }
        if (model != null && model.equals("32"))
            return "in3_jni_32";
        return "in3_jni";
    }

    private static byte[] md5(InputStream is) throws IOException {
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");
            md.reset();
            byte[] bytes = new byte[2048];
            int numBytes;
            while ((numBytes = is.read(bytes)) != -1)
                md.update(bytes, 0, numBytes);
            return md.digest();

        } catch (Exception ex) {
            throw new IOException(ex);
        } finally {
            is.close();
        }
    }

    public static void loadLibrary() {
        if (loaded)
            return;
        loaded = true;

        try {
            // try to load it from the path
            System.loadLibrary(getLibName());
            return;
        } catch (java.lang.UnsatisfiedLinkError x) {
        }

        // ok, not found, so we use the one in the package.

        String libFileName = System.mapLibraryName(getLibName());
        String jarPath = "/in3/native/" + libFileName;

        URL src = Loader.class.getResource(jarPath);
        if (src == null)
            throw new RuntimeException("Could not load the library for " + jarPath);

        try {
            File lib = new File(new File(System.getProperty("java.io.tmpdir")), libFileName);
            if (lib.exists() && !Arrays.equals(md5(src.openStream()), md5(new FileInputStream(lib))) && !lib.delete())
                throw new IOException(
                        "Could not delete the library from temp-file! Maybe some other proccess is still using it ");

            if (!lib.exists()) {
                InputStream is = null;
                OutputStream os = null;
                try {
                    is = src.openStream();
                    os = new FileOutputStream(lib);
                    byte[] buffer = new byte[4096];
                    int read = 0;
                    while ((read = is.read(buffer)) >= 0)
                        os.write(buffer, 0, read);
                } finally {
                    if (is != null)
                        is.close();
                    if (os != null)
                        os.close();
                }
                if (!System.getProperty("os.name").contains("Windows")) {
                    try {
                        Runtime.getRuntime().exec(new String[] { "chmod", "755", lib.getAbsolutePath() }).waitFor();
                    } catch (Throwable e) {
                    }
                }

            }
            System.load(lib.getAbsolutePath());

        } catch (Exception ex) {
            throw new RuntimeException("Could not load the native library ", ex);
        }
    }

}
