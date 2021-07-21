//add eeprom put and get functions (source https://www.stm32duino.com/viewtopic.php?t=314)

template <class T> void EEPROM_get (int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
}

template <class T> void EEPROM_put (int ee, T& value) {
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.update(ee++, *p++);
}
