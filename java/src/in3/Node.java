package in3;

import in3.eth1.Log;
import in3.utils.JSON;

public class Node {
  private JSON data;

  private Node(JSON data) {
    this.data = data;
  }

  protected static Node[] asNodes(Object o) {
    if (o == null)
      return null;
    if (o instanceof Object[]) {
      Object[] a = (Object[]) o;
      Node[] s   = new Node[a.length];
      for (int i = 0; i < s.length; i++)
        s[i] = a[i] == null ? null : new Node((JSON) a[i]);
      return s;
    }
    return null;
  }

  protected static Node asNode(Object o) {
    if (o == null)
      return null;
    return new Node((JSON) o);
  }

  public String getUrl() {
    return JSON.asString(data.get("url"));
  }

  public String getAddress() {
    return JSON.asString(data.get("address"));
  }

  public int getIndex() {
    return JSON.asInt(data.get("index"));
  }

  public String getDeposit() {
    return JSON.asString(data.get("deposit"));
  }

  public long getProps() {
    return JSON.asLong(data.get("props"));
  }

  public int getTimeout() {
    return JSON.asInt(data.get("timeout"));
  }

  public int getRegisterTime() {
    return JSON.asInt(data.get("registerTime"));
  }

  public int getWeight() {
    return JSON.asInt(data.get("weight"));
  }
}
