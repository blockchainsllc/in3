package in3;

import in3.utils.TransportException;
import java.io.*;
import java.net.*;

/*
 * Default implementation for HTTP transport in the java using java.net.HttpURLConnection
 */
class IN3DefaultTransport implements IN3Transport {

  @Override
  public byte[][] handle(String method, String[] urls, byte[] payload, String[] headers) throws TransportException {
    byte[][] result = new byte[urls.length][];

    for (int i = 0; i < urls.length; i++) {
      try {
        URL               url  = new URL(urls[i]);
        URLConnection     con  = url.openConnection();
        HttpURLConnection http = (HttpURLConnection) con;
        http.setRequestMethod(method);
        http.setUseCaches(false);
        http.setDoOutput(true);
        http.setRequestProperty("Content-Type", "application/json");
        http.setRequestProperty("Accept", "application/json");
        http.setRequestProperty("charsets", "utf-8");
        http.setRequestProperty("User-Agent", "in3 java " + IN3.getVersion());
        for (int n = 0; n < headers.length; n++) {
          int p = headers[n].indexOf(':');
          if (p > 0) http.setRequestProperty(headers[n].substring(0, p).trim(), headers[n].substring(p + 1).trim());
        }
        http.connect();
        OutputStream os = http.getOutputStream();
        os.write(payload);
        InputStream is = http.getInputStream();

        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        int                   nRead;
        byte[] data = new byte[1024];
        while ((nRead = is.read(data, 0, data.length)) != -1)
          buffer.write(data, 0, nRead);

        int status = http.getResponseCode();
        buffer.flush();
        is.close();
        if (status > 300)
          throw new TransportException(buffer.toString().replaceAll("\"", " "), status, i);
        else
          result[i] = buffer.toByteArray();
      } catch (Exception ex) {
        throw new TransportException("Error during request transport:" + ex.getMessage(), 11, 0);
      }
    }
    return result;
  }
}