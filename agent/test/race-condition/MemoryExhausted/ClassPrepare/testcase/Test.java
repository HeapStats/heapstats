import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;
import java.util.*;


public class Test{

  public static void runClassLoad(){
    try{
      ClassLoader loader = new URLClassLoader(new URL[]{
                     Paths.get(System.getProperty("java.class.path"), "dynload")
                          .toUri()
                          .toURL()});
      Class<?> target = loader.loadClass("DynLoad");
      Method targetMethod = target.getMethod("call");
      Object targetObj = target.newInstance();
      targetMethod.invoke(targetObj);
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  public static void runMemleak(){
    List<byte[]> list = new ArrayList<>();

    while(true){
      list.add(new byte[1024*1024]);
    }
  }

  public static void main(String[] args){
    (new Thread(Test::runClassLoad)).start();
    (new Thread(Test::runMemleak)).start();
  }
}
