import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;


public class Test{

  public static void consumeMemory(){
    while(true){
      System.gc();
      try{
        Thread.sleep(1000);
      }
      catch(Exception e){
        e.printStackTrace();
      }
    }
  }

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

  public static void runThreadleak(){
    Runnable waiter = () -> {
      try{
        synchronized(Test.class){
          Test.class.wait();
        }
      }
      catch(InterruptedException e){
        e.printStackTrace();
      }
    };

    while(true){
      try{
        (new Thread(waiter)).start();
      }
      catch(Throwable t){
        t.printStackTrace();
        System.exit(-1);
      }
    }
  }

  public static void main(String[] args){
    (new Thread(Test::runClassLoad)).start();
    (new Thread(Test::runThreadleak)).start();

    /* Avoid PythonException not to create native thread for executing jcmd */
    Thread th = new Thread(Test::consumeMemory);
    th.setDaemon(true);
    th.start();
  }
}
