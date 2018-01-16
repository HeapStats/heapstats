/*
 * Copyright (C) 2017 Yasumasa Suenaga
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */


public class Test implements Runnable{

  private final Object lock1;
  private final Object lock2;

  public Test(Object lock1, Object lock2){
    this.lock1 = lock1;
    this.lock2 = lock2;
  }

  public void run(){
    try{
      synchronized(lock1){
        Thread.sleep(3000);
        synchronized(lock2){
          System.out.println("Test error");
          System.exit(-1);
        }
      }
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  public static void main(String[] args) throws Exception{
    Object lock1 = new Object();
    Object lock2 = new Object();

    Thread th1 = new Thread(new Test(lock1, lock2));
    Thread th2 = new Thread(new Test(lock2, lock1));
    th1.setDaemon(true);
    th2.setDaemon(true);
    th1.start();
    th2.start();

    Thread.sleep(7000);
  }
}
