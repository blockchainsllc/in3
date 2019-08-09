package in3;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.*;
import java.security.DigestInputStream;
import java.security.NoSuchAlgorithmException;

public class Loader {

    private static boolean loaded = false;

    private static String md5(InputStream input) throws IOException {
        BufferedInputStream in = new BufferedInputStream(input);
        try {
            DigestInputStream dis = new DigestInputStream(in, java.security.MessageDigest.getInstance("MD5"));
            return new java.math.BigInteger(1, dis.getMessageDigest().digest()).toString(16);
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("MD5 algorithm is not available: " + e);
        } finally {
            in.close();
        }
    }

    public static void loadLibrary() {
        if (loaded)
            return;
        loaded = true;

        try {
            // try to load it from the path
            System.loadLibrary("in3");
            return;
        } catch (java.lang.UnsatisfiedLinkError x) {
        }

        // ok, not found, so we use the one in the package.

        String libFileName = System.mapLibraryName("in3");
        String jarPath = "/in3/native/" + libFileName;

        URL src = Loader.class.getResource(jarPath);
        if (src == null)
            throw new RuntimeException("Could not load the library for " + jarPath);

        try {
            File lib = new File(new File(System.getProperty("java.io.tmpdir")), libFileName);
            if (lib.exists() && !md5(src.openStream()).equals(md5(new FileInputStream(lib))) && !lib.delete())
                throw new IOException(
                        "Could not delete the library from temp-file! Maybe some other proccess is still using it ");

            if (!lib.exists())
                try (InputStream is = src.openStream(); OutputStream os = new FileOutputStream(lib)) {
                    is.transferTo(os);
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
