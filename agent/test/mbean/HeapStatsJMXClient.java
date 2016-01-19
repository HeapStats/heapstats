import com.sun.tools.attach.VirtualMachine;
import javax.management.remote.*;
import javax.management.*;
import java.net.*;
import java.io.*;
import java.nio.file.*;
import java.util.Optional;

import jp.co.ntt.oss.heapstats.mbean.HeapStatsMBean;


public class HeapStatsJMXClient{

  private static void test1(HeapStatsMBean mbean){
    System.out.println("Test1: HeapStats version");
    System.out.println(mbean.getHeapStatsVersion());
    System.out.println();
  }

  private static void test2(HeapStatsMBean mbean){
    System.out.println("Test2: Get Configuration");
    System.out.println("file = " + mbean.getConfiguration("file"));
    System.out.println();
  }

  private static void test3(HeapStatsMBean mbean){
    System.out.println("Test3: Change Configuration");

    System.out.print("file: ");
    System.out.print(mbean.getConfiguration("file").toString() + " -> ");
    mbean.changeConfiguration("file", "test.dat");
    System.out.println(mbean.getConfiguration("file").toString());

    System.out.print("loglevel: ");
    System.out.print(mbean.getConfiguration("loglevel").toString() + " -> ");
    mbean.changeConfiguration("loglevel", HeapStatsMBean.LogLevel.DEBUG);
    System.out.println(mbean.getConfiguration("loglevel").toString());

    System.out.print("rank_order: ");
    System.out.print(mbean.getConfiguration("rank_order").toString() + " -> ");
    mbean.changeConfiguration("rank_order", HeapStatsMBean.RankOrder.USAGE);
    System.out.println(mbean.getConfiguration("rank_order").toString());

    System.out.print("trigger_on_logsignal: ");
    System.out.print(mbean.getConfiguration("trigger_on_logsignal").toString()
                                                                      + " -> ");
    mbean.changeConfiguration("trigger_on_logsignal", Boolean.FALSE);
    System.out.println(mbean.getConfiguration(
                                      "trigger_on_logsignal").toString());

    System.out.print("alert_percentage: ");
    System.out.print(mbean.getConfiguration("alert_percentage").toString()
                                                                      + " -> ");
    mbean.changeConfiguration("alert_percentage", Integer.valueOf(30));
    System.out.println(mbean.getConfiguration("alert_percentage").toString());

    System.out.print("metaspace_alert_threshold: ");
    System.out.print(mbean.getConfiguration(
                              "metaspace_alert_threshold").toString() + " -> ");
    mbean.changeConfiguration("metaspace_alert_threshold", Long.valueOf(100));
    System.out.println(mbean.getConfiguration(
                                   "metaspace_alert_threshold").toString());

    System.out.println();
  }

  private static void test4(HeapStatsMBean mbean){
    System.out.println("Test4: Invoke Heap SnapShot collection");
    mbean.invokeSnapShotCollection();
    System.out.println("OK");
    System.out.println();
  }

  private static void test5(HeapStatsMBean mbean){
    System.out.println("Test5: Invoke Resource Log collection");
    System.out.println(mbean.invokeLogCollection());
    System.out.println();
  }

  private static void test6(HeapStatsMBean mbean){
    System.out.println("Test6: Invoke All Log collection");
    System.out.println(mbean.invokeAllLogCollection());
    System.out.println();
  }

  private static ServerSocket createServerSocket() throws IOException{
    SocketAddress addr = new InetSocketAddress(
                                      InetAddress.getLoopbackAddress(), 0);
    ServerSocket server = new ServerSocket();
    server.bind(addr);

    return server;
  }

  private static void receiveFile(String fname, ServerSocket server){
    try(OutputStream out = Files.newOutputStream(Paths.get(fname),
                                                 StandardOpenOption.CREATE)){
      byte[] buf = new byte[1024];
      int received;

      try(InputStream in = server.accept().getInputStream()){
        while((received = in.read(buf)) > 0){
          out.write(buf, 0, received);
        }
      }

    }
    catch(IOException e){
      throw new UncheckedIOException(e);
    }


  }

  private static void test7(HeapStatsMBean mbean)
                                   throws IOException, InterruptedException{
    System.out.println("Test7: Get SnapShot data");

    try(ServerSocket server = createServerSocket()){
      Thread th = new Thread(() -> receiveFile("received-snapshot.dat", server));
      th.start();

      mbean.getSnapShot(server.getInetAddress(), server.getLocalPort());
      th.join();
    }

    System.out.println("OK");
    System.out.println();
  }

  private static void test8(HeapStatsMBean mbean)
                                   throws IOException, InterruptedException{
    System.out.println("Test8: Get Resource log");

    try(ServerSocket server = createServerSocket()){
      Thread th = new Thread(() -> receiveFile("received-log.csv", server));
      th.start();

      mbean.getResourceLog(server.getInetAddress(), server.getLocalPort());
      th.join();
    }

    System.out.println("OK");
    System.out.println();
  }

  private static void test9(HeapStatsMBean mbean){
    System.out.println("Test9: All configurations");
    mbean.getConfigurationList()
         .forEach((k, v) -> System.out.println(k + " = " + v));
  }

  public static void main(String[] args) throws Exception{
    VirtualMachine targetVM = null;

    try{
      targetVM = VirtualMachine.attach(args[0]);
      JMXServiceURL jmxURL =
              new JMXServiceURL(targetVM.getAgentProperties().getProperty(
                         "com.sun.management.jmxremote.localConnectorAddress"));
      JMXConnector connector = JMXConnectorFactory.connect(jmxURL);
      MBeanServerConnection connection = connector.getMBeanServerConnection();
      HeapStatsMBean mbean = MBeanServerInvocationHandler.newProxyInstance(
                               connection,
                                 new ObjectName("heapstats:type=HeapStats"),
                                                  HeapStatsMBean.class, false);

      test1(mbean);
      test2(mbean);
      test3(mbean);
      test4(mbean);
      test5(mbean);
      test6(mbean);
      test7(mbean);
      test8(mbean);
      test9(mbean);
    }
    finally{
      Optional.ofNullable(targetVM)
              .ifPresent(v -> {
                                try{
                                  v.detach();
                                }
                                catch(IOException e){
                                  throw new UncheckedIOException(e);
                                }
                              });
    }

    System.out.println();
    System.out.println("All test is OK!");
    System.out.println();
  }

}

