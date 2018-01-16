
public class NoDeadLock implements Runnable{

  private Object lock;

  public NoDeadLock(Object lock){
    this.lock = lock;
  }

  public void run(){

    synchronized(lock){

      try{
        Thread.sleep(3000);
      }
      catch(Exception e){
        e.printStackTrace();
      }

      System.out.println("lock succeeded.");
    }

  }

  public static void main(String[] args) throws Exception{
    int locks = Integer.parseInt(args[0]);

    for(int idx = 0; idx < locks; idx++){
      (new Thread(new NoDeadLock(NoDeadLock.class))).start();
    }

  }

}

