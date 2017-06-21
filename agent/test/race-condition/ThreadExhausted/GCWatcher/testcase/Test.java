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
      try{
        System.gc();
        Thread.sleep(100);
      }
      catch(Throwable t){
        t.printStackTrace();
        System.exit(-1);
      }
    }
  }

  public static void main(String[] args){
    Thread threadleakThread = new Thread(Test::runThreadleak);
    Thread gcThread = new Thread(Test::runGC);
    gcThread.setDaemon(true);

    gcThread.start();
    threadleakThread.start();
  }
}
