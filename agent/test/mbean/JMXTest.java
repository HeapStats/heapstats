import java.lang.management.*;
import javax.management.*;

import jp.co.ntt.oss.heapstats.mbean.*;


public class JMXTest{

  public static void main(String[] args) throws Exception{
    MBeanServer server = ManagementFactory.getPlatformMBeanServer();
    server.registerMBean(new HeapStats(),
                                new ObjectName("heapstats:type=HeapStats"));
    System.out.println("MBean registration has succeeded.");
    System.out.println("Try to connect with JMX client (e.g. jconsole).");
    System.out.println("Sleep 10 min.");
    Thread.sleep(600000);
  }

}

