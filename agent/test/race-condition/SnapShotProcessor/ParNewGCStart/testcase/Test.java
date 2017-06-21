import java.util.*;

public class Test{

  public static void main(String[] args) throws Exception{
    System.gc();
    Thread.sleep(3000); // Sleep 3 secs to wait breakpoint...
    System.gc();

    List<byte[]> list = new ArrayList<>();
    for(int i = 1; i < 10; i++){
      list.add(new byte[1024 * 1024]); // 1MB
    }

  }

}
