import java.util.concurrent.locks.*;


public class Test implements Runnable{

  private Object lock;

  public Test(Object lock){
    this.lock = lock;
  }

  public void run(){

    synchronized(lock){
      try{
        Thread.sleep(1000);
        lock.wait(1000);
      }
      catch(InterruptedException e){
        // Do nothing.
      }
    }

  }

  public static void main(String[] args) throws Exception{
    Test test = new Test(new Object());
    
    (new Thread(test, "Test thread 1")).start();
    (new Thread(test, "Test thread 2")).start();

    Thread parker = new Thread(() -> LockSupport.park(), "Parker");
    parker.start();

    Thread.sleep(1000);

    LockSupport.unpark(parker);
  }

}

