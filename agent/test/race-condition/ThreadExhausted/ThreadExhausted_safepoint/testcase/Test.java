import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;
import java.util.*;


public class Test{

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

  public static void runGC(){
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

  public static void main(String[] args){
    (new Thread(Test::runThreadleak)).start();
    (new Thread(Test::runThreadleak)).start();

    Thread gcThread = new Thread(Test::runGC);
    gcThread.setDaemon(true);
    gcThread.start();
  }
}
