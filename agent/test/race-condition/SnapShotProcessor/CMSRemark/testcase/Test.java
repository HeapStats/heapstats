public class Test{

  public static void main(String[] args) throws Exception{
    System.gc();
    Thread.sleep(3000); // Sleep 3 secs to wait breakpoint...
    System.gc();
    Thread.sleep(10000); // Sleep 10 secs again to avoid missing...
    System.gc();
  }

}
