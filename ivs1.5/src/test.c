/*
 * Fill up the loop buffer to the size specified in lLimit.
 */
void FillLoopBuffer(pAserver, sdSound, sdSoundType, 
		    psSoundName, lBytesRead, lLimit, psBuf)
     AudioServer *pAserver;
     SoundId sdSound;
     aSoundType sdSoundType;
     char *psSoundName;
     long lBytesRead;
     long lLimit;
     char *psBuf;
{

  /* fill the loop buffer */
  while (glClientPos+lBytesRead > lLimit)
    printf("loop\n");
  
  APutSoundData(pAserver, sdSound, 1, glClientPos, sdSoundType, 
		lBytesRead, (SoundData) psBuf, ACurrentTime);
  glDataSent = (glClientPos += lBytesRead);

  /* just in case glClientPos is == glLoopBufSize */
  if (glClientPos == glLoopBufSize)
    glClientPos = 0;

}
/*
 * Handles CommandStart and SoundPosition event. 
 */
void HandleEvent(pAserver, pEvent)
	AudioServer *pAserver;
	aEvent *pEvent;
{
  aEvent event;

  ANextEvent(pAserver, &event);
  if(event.u.u.type==N_CommandStart) 
    {
      glPrevServPos = glCurrServPos = 0;
    } 
  else if(event.u.u.type==N_SoundPosition) 
    {
      glPrevServPos = glCurrServPos;
      glCurrServPos = (int)event.u.soundPositionN.position;
      glDataPlayed += MOD(glCurrServPos-glPrevServPos, glLoopBufSize);
      fprintf(stdout,".");
      fflush(stdout);
    }

  if (pEvent)
    bcopy(&event, pEvent, sizeof(aEvent));
}

/*
 * At each SoundPosition event coming back, the client will read 4000 bytes
 * and send them to the server. 
 */
void SendAtPosEvent(iFd, pAserver, sdSoundType, sdSound, psSoundName)
    int iFd;    register AudioServer *pAserver;
    aSoundType sdSoundType;
    SoundId   sdSound;
    char *psSoundName;
{
  aEvent event;
 
  Bool bDone = False;
  long lGot;

  SoundData psBuf = (SoundData )malloc(MAX_SOUND_SAMPLES);
    
  while(!bDone) 
    {
      HandleEvent(pAserver, &event);
      if(event.u.u.type==N_SoundPosition)
	{
	  /* read as much as possible into the buffer */
	  lGot = read(iFd, psBuf, MAX_SOUND_SAMPLES);

	  /* if end of file, break out the loop */
	  if (lGot == 0)
	    break;
	  else if (lGot==-1) 
	    {
	      fprintf(stderr, "Error on read of %s\n", psSoundName);
	      exit(-1);
	    }

	  /* flush the big buffer */
	  APutSoundData(pAserver, sdSound, SOUNDOP, 
			glClientPos, sdSoundType, lGot, 
			(SoundData) psBuf, ACurrentTime);
	  AFlush(pAserver);
	  glDataSent += lGot;
	  glClientPos = MOD(glClientPos+lGot,glLoopBufSize);
	}
    }

  free(psBuf);
}

