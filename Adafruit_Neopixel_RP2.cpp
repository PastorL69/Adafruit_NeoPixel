#if defined(ARDUINO_ARCH_RP2040)// RP2040 specific driver

#include "Adafruit_NeoPixel.h"

bool Adafruit_NeoPixel::rp2040claimPIO(void) {
  // Find a PIO with enough available space in its instruction memory
  pio = NULL;

  if (! pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, 
                                                         &pio, &pio_sm, &pio_program_offset, 
                                                         pin, 1, true)) {
    pio = NULL;
    pio_sm = -1;
    pio_program_offset = 0;
    return false; // No PIO available
  }

  // yay ok!
  
  if (is800KHz) {
    // 800kHz, 8 bit transfers
    ws2812_program_init(pio, pio_sm, pio_program_offset, pin, 800000, 8);
  } else {
    // 400kHz, 8 bit transfers
    ws2812_program_init(pio, pio_sm, pio_program_offset, pin, 400000, 8);
  }

  return true;
}

bool Adafruit_NeoPixel::rp2040claimDMA(void) {
  // Have a look at if any DMA channels are available
  dma_chan = dma_claim_unused_channel(false);
  if (dma_chan >= 0) {
    dma_cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_dreq(&dma_cfg, 
                            pio_get_dreq(pio, pio_sm, true));
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    dma_channel_configure(dma_chan, &dma_cfg,
                          &pio->txf[pio_sm],  // write addr
                          nullptr, 0, false); // will set src/len on start
    return true;
  }

  return false; // no DMA channel available :(
}

void Adafruit_NeoPixel::rp2040releaseDMA(void) {
  if (dma_chan == -1) 
    return;

  dma_channel_abort(dma_chan);
  dma_channel_unclaim(dma_chan);
  dma_chan = -1;
}

void Adafruit_NeoPixel::rp2040releasePIO(void) {
  if (pio == NULL) 
    return;

  pio_sm_clear_fifos(pio, pio_sm);
  pio_remove_program_and_unclaim_sm(&ws2812_program, pio, pio_sm,  pio_program_offset);
}


// Private, called from show()
void Adafruit_NeoPixel::rp2040Show(uint8_t *pixels, uint32_t numBytes)
{
  // verify we have a valid PIO and state machine
  if (! pio || (pio_sm < 0)) {
    return;
  }

  // if(dma_chan >= 0) {
  //   // set the read address and transfer immediately
  //   dma_channel_set_read_addr(dma_chan, pixels, false);
  //   dma_channel_set_trans_count(dma_chan, numBytes, true);
  //   return;
  // }

  while(numBytes--)
    // Bits for transmission must be shifted to top 8 bits
    pio_sm_put_blocking(pio, pio_sm, ((uint32_t)*pixels++)<< 24);
}
#endif
