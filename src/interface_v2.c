/*
 *
 * Copyright (C) 2017-2022 Maxime Schmitt <maxime.schmitt91@gmail.com>
 *
 * This file is part of Nvtop.
 *
 * Nvtop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nvtop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nvtop.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "nvtop/interface_v2.h"
#include "nvtop/common.h"
#include "nvtop/extract_gpuinfo_common.h"
#include "nvtop/interface_common.h"
#include "nvtop/interface_internal_common.h"
#include "nvtop/interface_layout_selection.h"
#include "nvtop/interface_options.h"
#include "nvtop/interface_ring_buffer.h"
#include "nvtop/interface_setup_win.h"
#include "nvtop/plot.h"
#include "nvtop/time.h"

#include <assert.h>
#include <inttypes.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <unistd.h>

static const char *PROCESS_TYPE_STRING[] = {"unknown", "graphical", "compute", "graphical_compute", "type_count"};

void print_escaped_string(char *s) {
    while (*s) {
        unsigned cp = (unsigned char)*s++;
        if (cp == 34 || cp == 92) {
            putchar(92);
        }
        putchar(cp);
    }
}

void print_snapshot_v2(struct list_head *devices) {
  gpuinfo_populate_static_infos(devices);
  gpuinfo_refresh_dynamic_info(devices);
  gpuinfo_refresh_processes(devices);
  // sleep(2);
  // gpuinfo_refresh_processes(devices);
  struct gpu_info *device;
  struct gpu_process *process;

  printf("[\n");
  list_for_each_entry(device, devices, list) {
    const char *indent_level_two = "  ";
    const char *indent_level_four = "   ";
    const char *indent_level_six = "     ";
    const char *indent_level_eight = "       ";

    const char *device_name_field = "device_name";
    const char *pdev_field = "pdev"; // e.g. 0000:01:00.0
    const char *gpu_clock_field = "gpu_clock";
    const char *mem_clock_field = "mem_clock";
    const char *temp_field = "temp";
    const char *fan_field = "fan_speed"; // RPM
    const char *fan_field_pct = "fan_speed_percentage";
    const char *power_field = "power_draw"; // mW
    const char *power_util_field = "power_util";
    const char *gpu_util_field = "gpu_util";
    const char *mem_util_field = "mem_util";
    const char *mem_used_field = "mem_used";
    const char *mem_free_field = "mem_free";
    const char *pcie_ingress_field = "pcie_ingress_rate"; // KB/s, traffic into the GPU
    const char *pcie_egress_field = "pcie_egress_rate";   // KB/s, traffic out of the GPU
    const char *encoder_util_field = "encoder_util";
    const char *decoder_util_field = "decoder_util";

    printf("%s{\n", indent_level_two);

    // Device Name
    if (GPUINFO_STATIC_FIELD_VALID(&device->static_info, device_name)) {
      printf("%s\"%s\": \"%s\",\n", indent_level_four, device_name_field, device->static_info.device_name);
    }

    // PDev
    if (strlen(device->pdev) > 0) {
      printf("%s\"%s\": \"%s\",\n", indent_level_four, pdev_field, device->pdev);
    }

    // GPU Clock Speed
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, gpu_clock_speed)) {
      printf("%s\"%s\": %u,\n", indent_level_four, gpu_clock_field, device->dynamic_info.gpu_clock_speed);
    }

    // MEM Clock Speed
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, mem_clock_speed)) {
      printf("%s\"%s\": %u,\n", indent_level_four, mem_clock_field, device->dynamic_info.mem_clock_speed);
    }

    // GPU Temperature, Celsius only
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, gpu_temp)) {
      printf("%s\"%s\": %u,\n", indent_level_four, temp_field, device->dynamic_info.gpu_temp);
    }

    // Fan speed, modified
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, fan_speed)) {
      printf("%s\"%s\": %u,\n", indent_level_four, fan_field_pct,
             device->dynamic_info.fan_speed > 100 ? 100 : device->dynamic_info.fan_speed);
    }
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, fan_rpm)) {
      printf("%s\"%s\": %u,\n", indent_level_four, fan_field,
             device->dynamic_info.fan_rpm > 9999 ? 9999 : device->dynamic_info.fan_rpm);
    }

    // Memory used, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, used_memory)) {
      printf("%s\"%s\": %llu,\n", indent_level_four, mem_used_field, device->dynamic_info.used_memory);
    }
    // Memory free, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, free_memory)) {
      printf("%s\"%s\": %llu,\n", indent_level_four, mem_free_field, device->dynamic_info.free_memory);
    }
    // Memory PCIe ingress, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, pcie_rx)) {
      printf("%s\"%s\": %u,\n", indent_level_four, pcie_ingress_field, device->dynamic_info.pcie_rx);
    }
    // Memory PCIe egress, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, pcie_tx)) {
      printf("%s\"%s\": %u,\n", indent_level_four, pcie_egress_field, device->dynamic_info.pcie_tx);
    }
    // Encoder util, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, encoder_rate)) {
      printf("%s\"%s\": %u,\n", indent_level_four, encoder_util_field, device->dynamic_info.encoder_rate);
    }
    // Decoder util, new
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, decoder_rate)) {
      printf("%s\"%s\": %u,\n", indent_level_four, decoder_util_field, device->dynamic_info.decoder_rate);
    }

    // Power draw
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, power_draw)) {
      printf("%s\"%s\": %u,\n", indent_level_four, power_field, device->dynamic_info.power_draw / 1000);
      if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, power_draw_max)) {
        printf("%s\"%s\": %u,\n", indent_level_four, power_util_field,
               (device->dynamic_info.power_draw * 100) / device->dynamic_info.power_draw_max);
      }
    }

    // GPU Utilization
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, gpu_util_rate)) {
      printf("%s\"%s\": %u,\n", indent_level_four, gpu_util_field, device->dynamic_info.gpu_util_rate);
    }
    // Memory Utilization
    if (GPUINFO_DYNAMIC_FIELD_VALID(&device->dynamic_info, mem_util_rate)) {
      printf("%s\"%s\": %u,\n", indent_level_four, mem_util_field, device->dynamic_info.mem_util_rate);
    }

    printf("%s\"processes\": [\n", indent_level_four);
    // sizeof(&device->processes) / sizeof(struct gpu_process)
    size_t i = 0;
    while (i < device->processes_count) {
      printf("%s{\n", indent_level_six);
      process = &device->processes[i];

      // Currently ignored fields:
      // Time-based fields:
      //  gfx_engine_used
      //  compute_engine_used
      //  enc_engine_used
      //  dec_engine_used
      // CPU fields:
      //  cpu_usage
      //  cpu_memory_virt
      //  cpu_memory_res
      // Meta field:
      //  sample_delta

      if (GPUINFO_PROCESS_FIELD_VALID(process, cmdline)) {
         printf("%s\"cmd\": \"", indent_level_eight);
         print_escaped_string(process->cmdline);
         printf("\",\n");
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, user_name)) {
        printf("%s\"username\": \"%s\",\n", indent_level_eight, process->user_name);
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, gpu_usage)) {
        printf("%s\"%s\": %llu,\n", indent_level_eight, gpu_util_field, process->gpu_usage); //
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, gpu_memory_usage)) {
        printf("%s\"%s\": %llu,\n", indent_level_eight, mem_used_field, process->gpu_memory_usage);
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, gpu_memory_percentage)) {
        printf("%s\"%s\": %u,\n", indent_level_eight, mem_util_field, process->gpu_memory_percentage);
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, gpu_cycles)) {
        printf("%s\"gpu_cycles\": %llu,\n", indent_level_eight, process->gpu_cycles);
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, encode_usage)) {
        printf("%s\"%s\": %llu,\n", indent_level_eight, encoder_util_field, process->encode_usage);
      }

      if (GPUINFO_PROCESS_FIELD_VALID(process, decode_usage)) {
        printf("%s\"%s\": %llu,\n", indent_level_eight, decoder_util_field, process->decode_usage);
      }

      printf("%s\"process_type\": \"%s\",\n", indent_level_eight, PROCESS_TYPE_STRING[process->type]); // useless?
      printf("%s\"pid\": \"%u\"\n", indent_level_eight, process->pid);

      printf("%s}", indent_level_six);

      if (++i < device->processes_count) {
        printf(",\n");
      } else {
        printf("\n");
      }
    }

    printf("%s]\n", indent_level_four);

    if (device->list.next == devices)
      printf("%s}\n", indent_level_two);
    else
      printf("%s},\n", indent_level_two);
  }
  printf("]\n");
}
