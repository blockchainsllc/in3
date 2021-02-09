package in3;

/*
 * Interface for the IN3 transport.
 */
public interface IN3Transport {
  byte[][] handle(String method, String[] urls, byte[] payload, String[] headers);
}