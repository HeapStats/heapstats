import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;

public class Test implements Runnable{

  public void run(){
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

  public static void main(String[] args) throws Exception{
    (new Thread(new Test())).start();
    (new Thread(new Test())).start();
  }
}
