package sun.misc;

import java.net.InetAddress;


public final class IoTrace{

  private IoTrace(){
    // Do nothing.
  }

  public static native Object socketReadBegin();

  public static native void socketReadEnd(Object context, InetAddress address,
                                         int port, int timeout, long bytesRead);

  public static native Object socketWriteBegin();

  public static native void socketWriteEnd(Object context, InetAddress address,
                                                   int port, long bytesWritten);

  public static native Object fileReadBegin(String path);

  public static native void fileReadEnd(Object context, long bytesRead);

  public static native Object fileWriteBegin(String path);

  public static native void fileWriteEnd(Object context, long bytesWritten);

}

