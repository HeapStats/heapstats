import java.util.*;


public class Test{

  public static void runMemleak(){
    List<byte[]> list = new ArrayList<>();

    while(true){
      list.add(new byte[1024*1024]);
    }
  }

  public static void main(String[] args){
    (new Thread(Test::runMemleak)).start();
  }
}
