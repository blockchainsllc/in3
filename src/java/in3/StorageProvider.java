package in3;

public interface StorageProvider {
    byte[] getItem(String key);
    void setItem(String key, byte[] content);
}
