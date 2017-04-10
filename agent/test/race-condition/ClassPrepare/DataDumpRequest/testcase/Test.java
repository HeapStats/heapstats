import java.util.stream.*;
import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;
import com.sun.tools.attach.*;
import sun.tools.attach.*;

public class Test implements Runnable{

  public void run(){
    try{
      String cp = Stream.of(System.getProperty("java.class.path").split(":"))
                        .filter(p -> !p.contains("tools.jar"))
                        .findAny()
                        .get();
      ClassLoader loader = new URLClassLoader(new URL[]{Paths.get(cp, "dynload")
                                                             .toUri()
                                                             .toURL()});
      Class<?> target = loader.loadClass("DynLoad");
      Method targetMethod = target.getMethod("call");
      Object targetObj = target.newInstance();
      targetMethod.invoke(targetObj);
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  public static void main(String[] args) throws Exception{
    (new Thread(new Test())).start();

    Path selfProc = Paths.get("/proc/self");
    String pid = Files.readSymbolicLink(selfProc).toString();
    HotSpotVirtualMachine selfVM =
                              (HotSpotVirtualMachine)VirtualMachine.attach(pid);
    try{
      selfVM.localDataDump();
    }
    finally{
      selfVM.detach();
    }

  }
}
