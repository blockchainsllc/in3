package in3;

import in3.utils.JSON;

public class NodeList {
    private JSON data;

    private NodeList(JSON data) {
        this.data = data;
    }

    protected static NodeList[] asNodeLists(Object o) {
        if (o == null)
            return null;
        if (o instanceof Object[]) {
            Object[] a = (Object[]) o;
            NodeList[] s = new NodeList[a.length];
            for (int i = 0; i < s.length; i++)
                s[i] = a[i] == null ? null : new NodeList((JSON) a[i]);
            return s;
        }
        return null;
    }

    protected static NodeList asNodeList(Object o) {
        if (o == null)
            return null;
        return new NodeList((JSON) o);
    }

    public Node[] getNodes() {
        Object obj = data.get("nodes");
        if (obj != null) {
            return Node.asNodes(obj);
        } else {
            return new Node[] {};
        }
    }
}
