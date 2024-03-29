/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

package in3.utils;

import in3.Loader;
import java.math.*;
import java.util.*;

/**
 * internal helper tool to represent a JSON-Object.
 *
 * Since the internal representation of JSON in incubed uses hashes instead of
 * name, the getter will creates these hashes.
 */
public class JSON {
  static {
    Loader.loadLibrary();
  }

  private HashMap<Integer, Object> map = new HashMap<Integer, Object>();

  protected static native int key(String name);

  public JSON() {}

  /**
   * gets the property
   *
   * @return the raw object.
   */
  public Object get(String prop /**
                                   the name of the property.
                                 */
  ) {
    return map.get(key(prop));
  }

  /**
   * adds values. This function will be called from the JNI-Iterface.
   *
   * Internal use only!
   */
  public void put(String key /**
                                the key
                              */
                  ,
                  Object val /**
                                the value object
                              */
  ) {
    map.put(key(key), val);
  }

  /**
   * adds values. This function will be called from the JNI-Iterface.
   *
   * Internal use only!
   */
  public void put(int key /**
                             the hash of the key
                           */
                  ,
                  Object val /**
                                the value object
                              */
  ) {
    map.put(key, val);
  }

  /**
   * returns the property as boolean
   *
   * @return the boolean value
   */
  public boolean getBoolean(String key /**
                                          the propertyName
                                        */
  ) {
    return asBoolean(get(key));
  }

  /**
   * returns the property as long
   *
   * @return the long value
   */
  public long getLong(String key /**
                                    the propertyName
                                  */
  ) {
    return asLong(get(key));
  }

  /**
   * returns the property as double
   *
   * @return the long value
   */
  public double getDouble(String key /**
                                        the propertyName
                                      */
  ) {
    return asDouble(get(key));
  }

  /**
   * returns the property as BigInteger
   *
   * @return the BigInteger value
   */
  public Object getObject(String key /**
                                        the propertyName
                                      */
  ) {
    return get(key);
  }

  /**
   * returns the property as BigInteger
   *
   * @return the BigInteger value
   */
  public Integer getInteger(String key /**
                                          the propertyName
                                        */
  ) {
    return asInt(get(key));
  }

  /**
   * returns the property as BigInteger
   *
   * @return the BigInteger value
   */
  public BigInteger getBigInteger(String key /**
                                                the propertyName
                                              */
  ) {
    return asBigInteger(get(key));
  }

  /**
   * returns the property as StringArray
   *
   * @return the array or null
   */
  public String[] getStringArray(String key /**
                                               the propertyName
                                             */
  ) {
    return asStringArray(get(key));
  }

  /**
   * returns the property as String or in case of a number as hexstring.
   *
   * @return the hexstring
   */
  public String getString(String key /**
                                        the propertyName
                                      */
  ) {
    return asString(get(key));
  }

  /**
   * casts the object to a String[]
   */
  public static String[] asStringArray(Object o) {
    if (o == null)
      return null;
    if (o instanceof Object[]) {
      Object[] a = (Object[]) o;
      String[] s = new String[a.length];
      for (int i = 0; i < s.length; i++)
        s[i] = a[i] == null ? null : asString(a[i]);
      return s;
    }
    return null;
  }

  public static BigInteger asBigInteger(Object o) {
    if (o == null)
      return BigInteger.valueOf(0);
    if (o instanceof String)
      return (((String) o).length() > 2 && o.toString().charAt(1) == 'x')
          ? new BigInteger(o.toString().substring(2), 16)
          : new BigInteger(o.toString(), 10);
    if (o instanceof Integer)
      return BigInteger.valueOf(((Integer) o).longValue());
    return BigInteger.valueOf(0);
  }

  public static long asLong(Object o) {
    if (o == null)
      return 0;
    if (o instanceof String)
      return (((String) o).length() > 2 && o.toString().charAt(1) == 'x')
          ? Long.parseLong(o.toString().substring(2), 16)
          : Long.parseLong(o.toString(), 10);
    if (o instanceof Integer)
      return ((Integer) o).longValue();
    return 0;
  }

  public static int asInt(Object o) {
    if (o == null)
      return 0;
    if (o instanceof String)
      return (((String) o).length() > 2 && o.toString().charAt(1) == 'x')
          ? Integer.parseInt(o.toString().substring(2), 16)
          : Integer.parseInt(o.toString(), 10);
    if (o instanceof Number)
      return ((Number) o).intValue();
    return 0;
  }

  public static double asDouble(Object o) {
    if (o == null)
      return 0;
    if (o instanceof Float) return ((Float) o).doubleValue();
    if (o instanceof Double) return ((Double) o).doubleValue();
    if (o instanceof String)
      return (((String) o).length() > 2 && o.toString().charAt(1) == 'x')
          ? JSON.asBigInteger(o).doubleValue()
          : Double.parseDouble(o.toString());
    if (o instanceof Number)
      return ((Number) o).doubleValue();
    return 0;
  }

  public static boolean asBoolean(Object o) {
    if (o == null)
      return false;
    if (o instanceof String)
      return Boolean.valueOf(o.toString());
    if (o instanceof Number)
      return !o.equals(0);
    return false;
  }

  public static String asString(Object o) {
    if (o == null)
      return null;
    if (o instanceof Integer)
      return "0x" + Integer.toHexString((Integer) o);
    if (o instanceof Long)
      return "0x" + Long.toHexString((Long) o);
    if (o instanceof BigInteger)
      return "0x" + ((BigInteger) o).toString(16);
    return o.toString();
  }

  public String toString() {
    return "<json-object>";
  }

  public static String toJson(Object ob) {
    if (ob == null)
      return "null";
    if (ob instanceof String)
      return "\"" + ob + "\"";
    if (ob instanceof BigInteger)
      return "\"0x" + ((BigInteger) ob).toString(16) + "\"";
    if (ob instanceof Number)
      return ob.toString();
    if (ob instanceof Boolean)
      return ob.toString();
    if (ob instanceof Object[]) {
      Object[] a       = (Object[]) ob;
      StringBuilder sb = new StringBuilder();
      sb.append("[");
      for (int i = 0; i < a.length; i++) {
        if (i > 0)
          sb.append(",");
        sb.append(toJson(a[i]));
      }
      return sb.append("]").toString();
    }
    return ob.toString();
  }

  public static void appendKey(StringBuilder sb, String key, Object value) {
    sb.append("\"").append(key).append("\":").append(toJson(value)).append(",");
  }

  public void addProperty(StringBuilder sb, String key) {
    Object o = get(key);
    if (o != null) addPropertyJson(sb, key, JSON.toJson(o));
  }

  public void addPropertyJson(StringBuilder sb, String key, String json) {
    if (json != null)
      sb.append(sb.length() == 1 ? "" : ",").append("\"").append(key).append("\":").append(json);
  }

  @Override
  public int hashCode() {
    final int prime  = 31;
    int       result = 1;
    result           = prime * result + ((map == null) ? 0 : map.hashCode());
    return result;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj)
      return true;
    if (obj == null)
      return false;
    if (getClass() != obj.getClass())
      return false;
    JSON other = (JSON) obj;
    if (map == null) {
      if (other.map != null)
        return false;
    }
    else if (!map.equals(other.map))
      return false;
    return true;
  }

  public <T> Map<String, T> asMap(Converter<Object, T> converter) {

    final HashMap<Integer, T> data = new HashMap<Integer, T>();
    for (Integer k : map.keySet())
      data.put(k, converter.apply(map.get(k)));
    return new HashMap<String, T>() {
      @Override
      public T get(Object key) {
        return data.get(JSON.key((String) key));
      }

      @Override
      public boolean containsKey(Object key) {
        return data.containsKey(JSON.key((String) key));
      }

      @Override
      public T put(String key, T value) {
        return data.put(JSON.key((String) key), value);
      }
    };
  }

  /**
   * parses a String to a json-object. If the json represents
   * - a object : JSON is returned
   * - a Array : a Array is returned
   * - other types the wrapped primative typed (Boolean, Integer, Long or String) will be returned.
   */
  public static native Object parse(String json) throws Exception;
}
