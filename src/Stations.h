const int totalStations = 13;    // if you add/delete stations change this accordingly

char *radioname[totalStations] = {  // Station names to be displayed on OLED (max 13 chars)
  "1. Groove Salad FM",
  "2. Nightride.FM", 
  "3. Underground 80s",  
  "4. Deep Space One", 
  "5. Awesome 80's", 
  "6. 90's Alternative",
  "7. Chilled Out",
  "8. Studio 181",
  "9. 181-fm",
  "10. Synphaera",
  "11. Poptron",
  "12. The Trip",
  "13. HeavyW Reggae"};

char * host[totalStations] = {
  "https://ice2.somafm.com/groovesalad-128-mp3", 
  "https://ice6.somafm.com/vaporwaves-128-mp3", 
  "https://ice4.somafm.com/u80s-128-mp3", 
  "https://ice1.somafm.com/deepspaceone-128-mp3", 
  "http://listen.livestreamingservice.com/181-awesome80s_128k.mp3", 
  "http://listen.livestreamingservice.com/181-90salt_128k.mp3",
  "http://listen.livestreamingservice.com/181-chilled_128k.mp3",
  "http://listen.livestreamingservice.com/181-ball_128k.mp3",
  "http://listen.livestreamingservice.com/181-jammin_128k.mp3",
  "https://ice2.somafm.com/synphaera-128-mp3",
  "https://ice2.somafm.com/poptron-128-mp3",
  "https://ice4.somafm.com/thetrip-128-mp3",
  "https://ice2.somafm.com/reggae-128-mp3"
};

