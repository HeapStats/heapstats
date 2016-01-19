/*
 * AgentAttacher.java
 * Created on 2013/3/19
 *
 * Copyright (C) 2011-2015 Nippon Telegraph and Telephone Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
package jp.co.ntt.oss.heapstats.attacher;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.File;
import java.util.List;

import com.sun.tools.attach.AgentInitializationException;
import com.sun.tools.attach.AgentLoadException;
import com.sun.tools.attach.AttachNotSupportedException;
import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;

public class AgentAttacher {

  public static void printAttachList(List<VirtualMachineDescriptor> vmlist) {
    /* Print vm list. */
    System.out.println("Num\tProcess");
    int idx = 0;

    for (VirtualMachineDescriptor vmd : vmlist) {
      System.out.printf("%d\t%s\n", idx++, vmd.displayName());
    }
  }
	
  public static void attachProcess(VirtualMachineDescriptor vmd,
                                   String agentPath, String options) throws
                                   IOException, AttachNotSupportedException, 
                              AgentLoadException, AgentInitializationException {
    VirtualMachine vm = null;

    try {
      /* attach to JVM. */
      if ((vm = VirtualMachine.attach(vmd)) == null) {
        throw new IllegalStateException("Could not attach to " +
                                        vmd.displayName());
      }

      /* load agent to JVM. */
      if (options == null) {
        vm.loadAgentPath(agentPath);
      } else {
        vm.loadAgentPath(agentPath, options);
      }
    } finally {
      if (vm != null) {
        vm.detach();
      }
    }
  }
	
  public static void main(String[] args) throws
                          IOException, AttachNotSupportedException,
                          AgentLoadException, AgentInitializationException {
    /* Check arguments. */
    if ((args.length == 0) || (args.length > 2) ||
        (args[0].equals("--help") || args[0].equals("-h"))) {
      System.out.println("Usage:");
      System.out.println("  AgentAttacher /path/to/agent [agent-options]");
      System.out.println("  AgentAttacher --help|-h");
      return;
    }

    /* Is exist agent file. */
    File agentFile = new File(args[0]);
    if (!agentFile.exists()) {
      System.err.println(agentFile.getAbsolutePath() + " does not exist.");
      return;
    }

    /* Copy strings from arguments. */
    String agent_path = args[0];
    String options = (args.length == 2) ? args[1] : null;

    /* Get vm list. */
    List<VirtualMachineDescriptor> vmlist = VirtualMachine.list();
    if (vmlist == null) {
      System.err.println("Java process not found.");
      return;
    }

    /* Print attachable java application list. */
    printAttachList(vmlist);

    /* Get target process number. */
    System.out.print("Process number to attach:");
    BufferedReader reader = null;

    try {
      reader = new BufferedReader(new InputStreamReader(System.in));
      int num = Integer.parseInt(reader.readLine());

      /* Attach process. */
      attachProcess(vmlist.get(num), agent_path, options);
      System.out.println("succeeded.");
    } finally {
      if (reader != null) {
        reader.close();
      }
    }
  }
}

