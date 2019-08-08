package in3;

import java.util.*;
import java.math.*;

public class JSON  {
   static {
      System.loadLibrary("in3"); 
   }
 
    private HashMap<Integer,Object> map = new HashMap<Integer,Object>();

    public native int key(String name);

    JSON() {    }
 
    public Object get(String prop) {
        return map.get(key(prop));
    }
    public void put(int key, Object val) {
        map.put(key,val);
    }

    public long getLong(String key) {
        return asLong(get(key));
    }
    public BigInteger getBigInteger(String key) {
        return asBigInteger( get(key) );
    }
   
   public String[] getStringArray(String key) {
        return astStringArray(get(key));
   }
   
   public String[] astStringArray(Object o) {
        if (o==null) return null;
        if (o instanceof Object[]) {
            Object[] a = (Object[]) o;
            String[] s = new String[a.length];
            for (int i=0;i<s.length;i++)
               s[i]=a[i]==null?null:a[i].toString();
            return s;
        } 
        return null;
    }

    public static BigInteger asBigInteger(Object o) {
        if (o==null) return BigInteger.valueOf(0);
        if (o instanceof String) return  new BigInteger(o.toString().substring(2),16);
        if (o instanceof Integer) return BigInteger.valueOf(((Integer) o).longValue());
        return BigInteger.valueOf(0);

    }


    public static long asLong(Object o) {
        if (o==null) return 0;
        if (o instanceof String) return Long.parseLong(o.toString().substring(2),16);
        if (o instanceof Integer) return ((Integer) o).longValue();
        return 0;
    }


    public String toString() {
        return "<json-object>";
    }
}
