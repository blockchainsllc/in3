package in3;

import in3.utils.JSON;
import java.util.HashMap;

public class IN3Props {

  HashMap<String, Object> props;

  private static final String DATA_NODES   = "dataNodes";
  private static final String SIGNER_NODES = "signerNodes";

  public IN3Props() {
    props = new HashMap<String, Object>();
  }

  public void setDataNodes(String[] adresses) {
    props.put(DATA_NODES, adresses);
  }

  public void setSignerNodes(String[] adresses) {
    props.put(SIGNER_NODES, adresses);
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("{");

    for (String key : props.keySet()) {
      JSON.appendKey(sb, key, props.get(key));
    }

    sb.setCharAt(sb.length() - 1, '}');
    return sb.toString();
  }

  public String toJSON() {
    return JSON.asString(this);
  }
}
