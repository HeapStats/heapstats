
public class DLSample implements Runnable{

  private Object lock1;
  private Object lock2;

  public DLSample(Object lock1, Object lock2){
    this.lock1 = lock1;
    this.lock2 = lock2;
  }

  public void run(){

    synchronized(lock1){

      try{
        Thread.sleep(1000);
      }
      catch(Exception e){
        e.printStackTrace();
      }

      synchronized(lock2){
        System.out.println("lock succeeded.");
      }

    }

  }

  public static void main(String[] args) throws Exception{
    int locks = Integer.parseInt(args[0]);
    Object[] lockObjs = new Object[locks];

    for(int idx = 0; idx < locks; idx++){
      lockObjs[idx] = new Object();
    }

    for(int idx = 0; idx < (locks - 1); idx++){
      (new Thread(new DLSample(lockObjs[idx], lockObjs[idx + 1]))).start();
    }

    (new Thread(new DLSample(lockObjs[locks - 1], lockObjs[0]))).start();
  }

}

