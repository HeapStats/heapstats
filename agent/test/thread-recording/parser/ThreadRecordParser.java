import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.nio.channels.*;
import java.util.*;
import java.time.*;


public class ThreadRecordParser{

  public static enum ThreadEvent{
    Unused,
    ThreadStart,
    ThreadEnd,
    MonitorWait,
    MonitorWaited,
    MonitorContendedEnter,
    MonitorContendedEntered,
    ThreadSleepStart,
    ThreadSleepEnd,
    Park,
    Unpark,
    FileWriteStart,
    FileWriteEnd,
    FileReadStart,
    FileReadEnd,
    SocketWriteStart,
    SocketWriteEnd,
    SocketReadStart,
    SocketReadEnd
  }

  public static class ThreadStat{

    private LocalDateTime time;

    private long id;

    private ThreadEvent event;

    private long additionalData;

    public ThreadStat(long longTime, long id, long longEvent,
                                                      long additionalData){
      Instant instant = Instant.ofEpochMilli(longTime);
      time = LocalDateTime.ofInstant(instant, ZoneId.systemDefault());
      this.id = id;
      event = ThreadEvent.values()[(int)longEvent];
      this.additionalData = additionalData;
    }

    public LocalDateTime getTime(){
      return time;
    }

    public long getId(){
      return id;
    }

    public ThreadEvent getEvent(){
      return event;
    }

    @Override
    public String toString(){
      return String.format("%s: %d: %s (%d)", time.toString(), id,
                                              event.toString(), additionalData);
    }

  }

  private ByteOrder getByteOrder(SeekableByteChannel ch, ByteBuffer buffer)
                                                             throws IOException{
    buffer.position(buffer.capacity() - 1);
    buffer.mark();
    ch.read(buffer);
    buffer.reset();

    return (buffer.get() == (byte)'L') ? ByteOrder.LITTLE_ENDIAN
                                       : ByteOrder.BIG_ENDIAN;
  }

  private Map<Long, String> getThreadIdMap(SeekableByteChannel ch,
                                          ByteBuffer buffer) throws IOException{
    int capacity = buffer.capacity();

    buffer.position(capacity - 4);
    buffer.mark();
    ch.read(buffer);
    buffer.reset();

    int mapSize = buffer.getInt();
    Map<Long, String> idMap = new HashMap<>();

    for(int Cnt = 0; Cnt < mapSize; Cnt++){
      buffer.position(capacity - 8);
      buffer.mark();
      ch.read(buffer);
      buffer.reset();
      long id = buffer.getLong();

      buffer.position(capacity - 4);
      buffer.mark();
      ch.read(buffer);
      buffer.reset();
      int len = buffer.getInt();

      ByteBuffer utf8Buffer = ByteBuffer.allocate(len);
      ch.read(utf8Buffer);
      utf8Buffer.flip();
      String className = new String(utf8Buffer.array());

      idMap.put(id, className);
    }

    return idMap;
  }

  private List<ThreadStat> getThreadStatList(SeekableByteChannel ch,
                                          ByteBuffer buffer) throws IOException{
    List<ThreadStat> threadStatList = new ArrayList<>();
    buffer.flip();

    while(ch.read(buffer) != -1){
      buffer.flip();
      ThreadStat stat = new ThreadStat(buffer.getLong(), buffer.getLong(),
                                        buffer.getLong(), buffer.getLong());

      if(stat.getEvent() == ThreadEvent.Unused){
        break;
      }

      threadStatList.add(stat);
      buffer.flip();
    }

    return threadStatList;
  }

  private void parse(Path path) throws IOException{
    ByteBuffer buffer = ByteBuffer.allocateDirect(32);

    try(SeekableByteChannel ch = Files.newByteChannel(path,
                                                      StandardOpenOption.READ)){
      buffer.order(getByteOrder(ch, buffer));
      System.out.println("ByteOrder: " + buffer.order());
      System.out.println();

      Map<Long, String> idMap = getThreadIdMap(ch, buffer);
      System.out.println("Thread ID Map:");
      idMap.forEach((k, v) -> System.out.println(k + ": " + v));
      System.out.println();

      List<ThreadStat> threadStatList = getThreadStatList(ch, buffer);
      threadStatList.forEach(System.out::println);
    }

  }

  public static void main(String[] args) throws Exception{
    ThreadRecordParser parser = new ThreadRecordParser();
    parser.parse(Paths.get(args[0]));
  }

}

