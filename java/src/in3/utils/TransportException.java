package in3.utils;
/**
 * Exception to be thrown in case of failed request.
 *
 */
public class TransportException extends Exception {
  private int status;
  private int index;

  /**
   * constrcutor
   * @param message
   * @param status
   */
  public TransportException(String message, int status, int index) {
    super(message);
    this.status = status;
    this.index  = index;
  }

  public int getIndex() {
    return index;
  }

  /**
   * the http-status
   * @return
   */
  public int getStatus() {
    return status;
  }
}
