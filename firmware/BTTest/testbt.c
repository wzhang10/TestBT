// Application-layer Bluetooth logic.

#include <string.h>
#include <stdio.h>

#include "testbt.h"
#include "log.h"
#include "hci.h"
#include "l2cap.h"
#include "rfcomm.h"
#include "sdp.h"
#include "btstack_memory.h"
#include "hci_transport.h"
#include "btstack/sdp_util.h"

static void DummyCallback(int h, const void *data, UINT32 size) {
}

static uint8_t    rfcomm_channel_nr = 1;
static uint16_t   rfcomm_channel_id;
static uint8_t    spp_service_buffer[128] __attribute__((aligned(__alignof(service_record_item_t))));
static uint8_t    rfcomm_send_credit = 0;
static BTCallback client_callback;
static char       local_name[] = "KM_BT_TAG (00:00)";  // the digits will be replaced by the MSB of the BD-ADDR
static char       sdp_name[] = "KMToken";
static char       pin_code[] = "5656";
int rfcomm_channel = 1;
uint8_t           addr[] = {0x43, 0x25, 0xD1, 0x00, 0x00, 0x00};


static void PacketHandler(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  bd_addr_t event_addr;
  uint8_t rfcomm_channel_nr;
  uint16_t mtu;

  switch (packet_type) {
    case HCI_EVENT_PACKET:
      switch (packet[0]) {
        case BTSTACK_EVENT_STATE:
          // bt stack activated, get started - set local name
          if (packet[2] == HCI_STATE_WORKING) {
            hci_send_cmd(&hci_write_local_name, local_name);
          }
          break;

        case HCI_EVENT_COMMAND_COMPLETE:
          if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
            bt_flip_addr(event_addr, &packet[6]);
            log_printf("BD-ADDR: %s\n\r", bd_addr_to_str(event_addr));
            sprintf(local_name, "KM_BT_TAG (%02X:%02X)", event_addr[4], event_addr[5]);
            break;
          }
          if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
            hci_discoverable_control(1);
            break;
          }
          break;

        case HCI_EVENT_LINK_KEY_REQUEST:
          // deny link key request
          log_printf("Link key request\n\r");
          bt_flip_addr(event_addr, &packet[2]);
          hci_send_cmd(&hci_link_key_request_negative_reply, &event_addr);
          break;

        case HCI_EVENT_PIN_CODE_REQUEST:
          // inform about pin code request
          // get this when pairing, good time to save device address if needed
          log_printf("Pin code request - using '%s'\n\r", pin_code);
          bt_flip_addr(event_addr, &packet[2]);
          hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, pin_code);
          break;

        case RFCOMM_EVENT_INCOMING_CONNECTION:
          // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
          bt_flip_addr(event_addr, &packet[2]);
          rfcomm_channel_nr = packet[8];
          rfcomm_channel_id = READ_BT_16(packet, 9);
          log_printf("RFCOMM channel %u requested for %s\n\r", rfcomm_channel_nr, bd_addr_to_str(event_addr));
          rfcomm_accept_connection_internal(rfcomm_channel_id);
          break;

        case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
          // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
          if (packet[2]) {
            log_printf("RFCOMM channel open failed, status %u\n\r", packet[2]);
          } else {
            rfcomm_channel_id = READ_BT_16(packet, 12);
            rfcomm_send_credit = 1;
            mtu = READ_BT_16(packet, 14);
            log_printf("\n\rRFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n\r", rfcomm_channel_id, mtu);
          }
          break;

        case RFCOMM_EVENT_CHANNEL_CLOSED:
          log_printf("RFCOMM channel closed.");
          client_callback(rfcomm_channel_id, NULL, 0);
          rfcomm_channel_id = 0;
          break;

        default:
          break;
      }
      break;

    case RFCOMM_DATA_PACKET:
      client_callback(rfcomm_channel_id, packet, size);
      rfcomm_send_credit = 1;

    default:
      break;
  }
}

void ClientPacketHandler(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    bd_addr_t event_addr;
    uint16_t mtu;
    //uint16_t rfcomm_channel_id;

    switch (packet_type) {

        case RFCOMM_DATA_PACKET:
            log_printf("Received RFCOMM data on channel id %u, size %u\n", channel, size);
            hexdump(packet, size);
            break;

        case HCI_EVENT_PACKET:
            switch (packet[0]) {

                case BTSTACK_EVENT_POWERON_FAILED:
                    // handle HCI init failure
                    log_printf("HCI Init failed - make sure you have turned off Bluetooth in the System Settings\n");
                    exit(1);
                    break;

                case BTSTACK_EVENT_STATE:
                    // bt stack activated, get started
                    if (packet[2] == HCI_STATE_WORKING) {
                        log_printf("btstack activated\n\r");
                        hci_send_cmd(&hci_write_local_name, local_name);
                        log_printf("btstack activated2\n\r");
                        //rfcomm_create_channel_internal(connection, addr, rfcomm_channel);
                    }
                    break;

                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    log_printf("Using PIN 0000\n\r");
                    bt_flip_addr(event_addr, &packet[2]);
                    hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
                    break;

                case HCI_EVENT_COMMAND_COMPLETE:
                      if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
                        bt_flip_addr(event_addr, &packet[6]);
                        log_printf("BD-ADDR: %s\n\r", bd_addr_to_str(event_addr));
                        sprintf(local_name, "KM_BT_TAG (%02X:%02X)", event_addr[4], event_addr[5]);
                        break;
                      }
                      if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
                        hci_discoverable_control(1);
                        rfcomm_create_channel_internal(connection, addr, rfcomm_channel);
                        break;
                      }
                      break;

                case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
                    // data: event(8), len(8), status (8), address (48), handle(16), server channel(8), rfcomm_cid(16), max frame size(16)
                    if (packet[2]) {
                        log_printf("RFCOMM channel open failed, status %u\n\r", packet[2]);
                    } else {
                        rfcomm_channel_id = READ_BT_16(packet, 12);
                        mtu = READ_BT_16(packet, 14);
                        log_printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n\r", rfcomm_channel_id, mtu);
                    }
                    break;

                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    // connection closed -> quit test app
                    log_printf("Basebank connection closed\n\r");
                    break;

                default:
                    break;
            }
            break;
        default:
            break;
    }
}



void BTInit() {
  btstack_memory_init();

  // init HCI
  hci_transport_t * transport = hci_transport_mchpusb_instance();
  bt_control_t * control = NULL;
  hci_uart_config_t * config = NULL;
  remote_device_db_t * remote_db = NULL;
  hci_init(transport, config, control, remote_db);

  // init L2CAP
  l2cap_init();
  l2cap_register_packet_handler(ClientPacketHandler);

  // init RFCOMM
  rfcomm_init();
  rfcomm_register_packet_handler(ClientPacketHandler);
  //rfcomm_register_service_internal(NULL, rfcomm_channel_nr, 100); // reserved channel, mtu=100

  // init SDP, create record for SPP and register with SDP
  sdp_init();
  memset(spp_service_buffer, 0, sizeof (spp_service_buffer));
  service_record_item_t * service_record_item = (service_record_item_t *) spp_service_buffer;
  sdp_create_spp_service((uint8_t*) & service_record_item->service_record, 1, sdp_name);
  log_printf("SDP service buffer size: %u\n\r", (uint16_t) (sizeof (service_record_item_t) + de_get_len((uint8_t*) & service_record_item->service_record)));
  sdp_
  sdp_register_service_internal(NULL, service_record_item);

  hci_power_control(HCI_POWER_ON);

  client_callback = DummyCallback;
}

void BTShutdown() {
  hci_close();
}

void BTTasks() {
  hci_transport_mchpusb_tasks();

  if (rfcomm_channel_id && rfcomm_send_credit) {
    rfcomm_grant_credits(rfcomm_channel_id, 1);
    rfcomm_send_credit = 0;
  }
}

int BTAccepting() {
  return rfcomm_channel_id != 0;
}

void BTSetCallback(BTCallback cb) {
  client_callback = cb;
}

void BTWrite(const void *data, int size) {
  assert(!(size >> 16));
  rfcomm_send_internal(rfcomm_channel_id, (uint8_t *) data, size & 0xFFFF);
}

int BTCanWrite() {
  return rfcomm_can_send(rfcomm_channel_id);
}

void BTClose() {
  rfcomm_disconnect_internal(rfcomm_channel_id);
}


void BTConnectToServer(){
    hci_discoverable_control(1);
    hci_send_cmd(&rfcomm_create_channel, addr, rfcomm_channel);
}
