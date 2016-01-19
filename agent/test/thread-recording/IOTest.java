import java.io.*;
import java.net.*;
import java.util.*;
import java.nio.file.*;
import java.nio.charset.*;


public class IOTest implements Runnable{

  private int port;

  public int getPort(){
    return port;
  }

  public void run(){

    try(ServerSocket server = new ServerSocket(0, 1,
                                      InetAddress.getLoopbackAddress())){
      port = server.getLocalPort();

      try(Socket sock = server.accept();
          BufferedReader reader = new BufferedReader(new InputStreamReader(
                                                      sock.getInputStream()));
          PrintWriter writer = new PrintWriter("test.txt")){

        String line;
        while((line = reader.readLine()) != null){
          writer.println(line);
        }

      }

    }
    catch(Exception e){
      e.printStackTrace(); 
    }

  }

  public static void main(String[] args) throws Exception{
    IOTest test = new IOTest();
    Thread writerThread = new Thread(test, "Writer thread");
    writerThread.start();

    Thread.sleep(1000);

    List<String> lines = Files.readAllLines(Paths.get(args[0]),
                                                   StandardCharsets.UTF_8);

    try(Socket sock = new Socket(InetAddress.getLoopbackAddress(),
                                                       test.getPort());
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(
                                                  sock.getOutputStream()))){
      for(String line : lines){
        writer.println(line);
      }

    }

  }

}

