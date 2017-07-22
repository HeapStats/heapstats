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

  private final Object lock;

  public Test(Object lock){
    this.lock = lock;
  }

  public void run(){
    try{
      synchronized(lock){
        Thread.sleep(3000);
      }
    }
    catch(Exception e){
      e.printStackTrace();
    }
  }

  public static void main(String[] args) throws Exception{
    Object lock1 = new Object();
    Object lock2 = new Object();

    Thread th1 = new Thread(new Test(lock1));
    Thread th2 = new Thread(new Test(lock1));
    th1.start();
    th2.start();

    Thread th3 = new Thread(new Test(lock2));
    Thread th4 = new Thread(new Test(lock2));

    th3.start();
    th4.start();

    th1.join();
    th2.join();
    th3.join();
    th4.join();
  }
}
