import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;


public class Test{

  public static void main(String[] args) throws Exception{
    (new Thread(() -> System.out.println("from new thread"))).start();

    ClassLoader loader = new URLClassLoader(new URL[]{
                     Paths.get(System.getProperty("java.class.path"), "dynload")
                          .toUri()
                          .toURL()});

    Class<?> target = loader.loadClass("DynLoad");
    Method targetMethod = target.getMethod("call");
    Object targetObj = target.newInstance();
    targetMethod.invoke(targetObj);
  }

}
