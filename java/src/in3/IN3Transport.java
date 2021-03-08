package in3;

import in3.utils.TransportException;

/*
 * Interface for the IN3 transport.
 */
public interface IN3Transport {
  byte[][] handle(String method, String[] urls, byte[] payload, String[] headers) throws TransportException;
}