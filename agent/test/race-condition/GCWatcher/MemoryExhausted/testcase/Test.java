import java.util.*;


public class Test{

  public static void runMemleak(){
    List<byte[]> list = new ArrayList<>();

    while(true){
      list.add(new byte[1024*1024]);
    }
  }

  public static void runGC(){
    try{
      while(true){
        System.gc();
        Thread.sleep(100);
      }
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  public static void main(String[] args){
    Thread gcThread = new Thread(Test::runGC);
    gcThread.setDaemon(true);
    gcThread.start();
    (new Thread(Test::runMemleak)).start();
  }
}
