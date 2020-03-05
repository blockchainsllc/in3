package in3;

import java.io.*;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.Files;
import java.util.*;
import org.json.*;

class IN3MockTransport implements IN3Transport {

  private static final String PROPERTIES_FILENAME = "config.properties";
  private static final String RESPONSES_DIR       = "responsesDir";

  private static final int    FIRST_INDEX = 0;
  private static final String METHOD      = "method";

  public Map<String, byte[]> requestToResponse;

  public IN3MockTransport() {
    requestToResponse = new HashMap<String, byte[]>();
  }

  @Override
  public byte[][] handle(String[] urls, byte[] payload) {
    byte[][] result   = new byte[urls.length][];
    String methodName = this.getPayloadMethod(new String(payload));

    if (!"".equals(methodName)) {
      for (int i = 0; i < urls.length; i++) {
        // Assumes a single payload to be requested to multiple nodes
        result[i] = requestToResponse.get(methodName);
      }
    }

    return result;
  }

  public void clearResponses() {
    requestToResponse.clear();
  }

  public void setMockResponse(String key, String fileName) {
    try {
      File folder   = new File(getResponsesFolder());
      File mockFile = new File(folder, fileName);
      setMockResponseFromFile(key, mockFile);
    } catch (Throwable ignored) {
    }
  }

  private String getPayloadMethod(String payload) {
    JSONArray arr = new JSONArray(payload);
    if (arr != null && arr.length() > 0) {
      return arr.getJSONObject(FIRST_INDEX).getString(METHOD);
    }

    return "";
  }

  private boolean setMockResponseFromFile(String key, File source) {
    try {
      if (source.isFile() && source.canRead()) {
        requestToResponse.put(key, this.getFileContents(source));
        return true;
      }
    } catch (IOException ignored) {
    }
    return false;
  }

  private URI getResponsesFolder() throws IOException, URISyntaxException {
    return this.getClass().getClassLoader().getResource(this.getProperty(RESPONSES_DIR)).toURI();
  }

  private byte[] getFileContents(File file) throws IOException {
    return Files.readAllBytes(file.toPath());
  }

  private String getProperty(String key) throws IOException {
    Properties props = new Properties();
    props.load(this.getFileResource(PROPERTIES_FILENAME));
    return props.getProperty(key);
  }

  private InputStream getFileResource(String fileName) {
    return this.getClass().getClassLoader().getResourceAsStream(fileName);
  }
}