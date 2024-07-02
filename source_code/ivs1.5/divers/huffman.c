/**************************************************************************\
*                                                                          *
*  Fichier : huffman.c                                                     *
*  Date    : 23 Decembre 1991                                              *
*  Auteur  : Thierry Turletti                                              *
*  Release : 1.0.0                                                         *
*--------------------------------------------------------------------------*
*  Description :  Procedures d'initialisation des tables de codage de      *
*               Huffman. Celle des coefficients de FCT est traitee a part  *
*               car le fichier d'entree n'est pas trie.                    *
*                En entree : Le fichier de donnees contenant les mots codes*
*               avec leur signification correspondante trie selon le champ *
*               result dans l'ordre croissant.                             *
*               En sortie :                                                *
*                  - Un tableau tab_code de mot_code trie dans l'ordre     *
*               croissant. (mot_code = struct{        --> Exemple          *
*                                        int result;  --> 23               *
*                                        char *chain; --> "01"             *
*                                      }). Ce tableau est utilise par le   *
*               codeur pour l'encodage au format H.261.                    *
*                  - Un tableau state d'etats utilise pour le decodage     *
*               H.261 et initialise de la maniere suivante :               *
*                  etat = struct{                                          *
*                           int bit[2];                                    *
*                           int result[2];                                 *
*                         }. Soit x l'etat courant: bit[0] est initialise  *
*               avec l'indice du tableau state correspondant a l'etat x0.  *
*               et bit[1] avec l'indice du tableau state correspondant a   *
*               l'etat x1. Si xi est un etat final, i.e un mot_code, alors *
*               bit[i] = 0 et result[i] = result correspondant au mot_code.*
*               Avec l'exemple tres simple suivant :                       *
*               mot_code   result associe                                  *
*                   1            25                                        *
*                   00           76                                        *
*                   01           23                                        *
*               # state[0] : (etat initial "" aucune info)                 *
*                    bit[0] = 1; (etat "0")                                *
*                    bit[1] = 0; (etat final "1")                          *
*                    result[1] = 25;                                       *
*                                                                          *
*               # state[1] : (etat "0")                                    *
*                    bit[0] = 0; (etat final "00")                         *
*                    bit[1] = 0; (etat final "01")                         *
*                    result[0] = 76;                                       *
*                    result[1] = 23;                                       *
*                                                                          *
\**************************************************************************/

#include "h261.h"


#define get_bit(octet,i,j) \
    (j==7 ? (j=0,(octet[(i)++]>>7)&0x01) : (octet[i]>>((j)++))&0x01)





int create_vlc_tcoeff(state, tab_code)
  /*
  *  Fonction qui cree a partir d'un fichier de texte contenant les
  *  codes de Huffman et leurs resultats correspondants :
  *    - Un tableau de structures "state" d'allure arborescente. Ce dernier
  *  est utilise par le decodeur et contient tous les mots codes terminaux
  *  ou non avec leurs resultats associes.
  *    - Un tableau de structures "mot_code" contenant tous les mots-codes
  *  terminaux et leur resultats associes. Ce tableau, trie dans l'ordre
  *  croissant, est utilise par le codeur.
  *  Cette fonction rend le nombre de codes terminaux.
  */

  etat *state;
  mot_code *tab_code;
  {
    FILE *table;
    char ccode[L_MAX], code[L_MAX];
    int run, level, inc, result;
    int longueur; /* longueur totale du mot code courant */
    int i, j, k, l, fin;
    int n_code = 0; /* Nombre de mots code trouves */
    int next_bit;
    int nb_entries = 1; /* Nombre d'entrees de la table, on compte la racine */
    char mot[L_STATE][L_MAX], chain[L_MAX];

      for(i=0; i<L_STATE; i++){
	state[i].bit[0] = -99;
	state[i].bit[1] = -99;
	state[i].result[0] = -99;
	state[i].result[1] = -99;
      }
      if((table=fopen("VLC.TCOEFF","r"))==NULL){
	fprintf(stderr,"Cannot open file VLC.TCOEFF\n");
        exit(-1);
      }
      do
      {
        fin = fscanf(table,"%s %d %d ",ccode, &run, &level);
	/********************\
	* Calcul du resultat *
	\********************/
	if(run>0)
	    result = (run<<4) + level;
	else
	    result = level; /* state = EOB ou ESCAPE */
	longueur = 0;
	while(ccode[longueur] != '\0')
	    longueur ++;
	for(l=0; l<longueur-1; l++){
	  /* Parcours de tous les state du state final ccode[longueur] */
	  code[l] = ccode[l];
	  code[l+1] = '\0';
	  next_bit = (ccode[l+1]=='0' ? 0 : 1);
	  for(i=1; i<nb_entries; i++)
	      if(strcmp(code,mot[i])==0){
		/* Ce mot code est connu */
		if(state[i].bit[next_bit] == -99){
		    /* Cette branche n'est pas encore initialisee */
		    if(l==longueur-2){
			/* et c'est un state final */
			state[i].bit[next_bit] = 0;
			state[i].result[next_bit] = result;
			strcpy(chain,mot[i]);
			strcat(chain,(next_bit ? "1" : "0"));
			tab_code[n_code].result = result;
			strcpy(tab_code[n_code].chain,chain);
			n_code ++;
		      }else /* et ce n'est pas un state final */
			  state[i].bit[next_bit] = nb_entries;
		}
		break;
	      }
          if(i == nb_entries){
	      /* c'est un nouveau state */
	      strcpy(mot[nb_entries], code);
	      if(l==longueur-2){
		/* C'est un state final */
		state[i].bit[next_bit] = 0;
		state[i].result[next_bit] = result;
		strcpy(chain,mot[i]);
		strcat(chain,(next_bit ? "1" : "0"));
		tab_code[n_code].result = result;
		strcpy(tab_code[n_code].chain,chain);
		n_code ++;
		nb_entries ++;
	      }else /* state n'est pas final */
		  state[i].bit[next_bit] = ++nb_entries;
	  }
	}
      } while (fin != EOF);
    fclose(table);
    /* Il reste a initialiser la racine ... */
    for(i=1; i<nb_entries; i++)
      if(mot[i][0] == '0'){
	  /* mot[i] est le 1er mot de l'arbre commencant par 0 */
	  state[0].bit[0] = i;
	  break;
      }
    for(i=1; i<nb_entries; i++)
      if(mot[i][0] == '1'){
          /* mot[i] est le 1er mot de l'arbre commencant par 1 */
          state[0].bit[1] = i;
	  break;
      }
    /* ... et a trier en ordre croissant tab_code */
    /* Tri Shell (N.log(N)) */
    inc = 1;
    while(inc<(n_code/9))
	inc = inc*3 + 1;
    do{
      for(k=1; k<=inc; k++)
	  for(i=(inc+k-1); i<n_code; i+=inc){
	    strcpy(chain,tab_code[i].chain);
	    result = tab_code[i].result;
	    j = i - inc;
	    while((j>=0) && (tab_code[j].result>result)){
	      strcpy(tab_code[j+inc].chain,tab_code[j].chain);
	      tab_code[j+inc].result = tab_code[j].result;
	      j -= inc;
	    }
	    strcpy(tab_code[j+inc].chain,chain);
	    tab_code[j+inc].result = result;
	  }
      inc /= 3;
    }while(inc);
/*    fprintf(stderr,"\n\t\t\t*** FILE VLC.TCOEFF ***\n");
    for(l=0; l<nb_entries; l++)
	fprintf(stderr,"state[%3d]= %s *** 0-> %d, 1-> %d ** 0: %d, 1: %d\n",
                 l,mot[l],state[l].bit[0],state[l].bit[1],state[l].result[0],
                 state[l].result[1]);
    printf("state de VLC.TCOEFF a %d elements\n",nb_entries);
*/
    return(n_code);
  }



int create_vlc(name_file, state, tab_code)
  /*
  *  Fonction qui cree a partir d'un fichier de texte contenant les
  *  codes de Huffman et leurs resultats correspondants :
  *    - Un tableau de structures "state" d'allure arborescente. Ce dernier
  *  est utilise par le decodeur et contient tous les mots codes terminaux
  *  ou non avec leurs resultats associes.
  *    - Un tableau de structures "mot_code" contenant tous les mots-codes
  *  terminaux et leur resultats associes. Ce tableau, trie dans l'ordre
  *  croissant, est utilise par le codeur.
  *  Cette fonction rend le nombre de codes terminaux.
  */

  char *name_file;
  etat *state;
  mot_code *tab_code;
  {
    FILE *table;
    char ccode[L_MAX], code[L_MAX];
    int result;
    int longueur; /* longueur totale du mot code courant */
    int i, l, fin;
    int n_code = 0; /* Nombre de mots code trouves */
    int next_bit;
    int nb_entries = 1; /* Nombre d'entrees de la table, on compte la racine */
    char mot[L_STATE][L_MAX], chain[L_MAX];

      for(i=0; i<L_STATE; i++){
	state[i].bit[0] = -99;
	state[i].bit[1] = -99;
	state[i].result[0] = -99;
	state[i].result[1] = -99;
      }
      if((table=fopen(name_file,"r"))==NULL){
	fprintf(stderr,"Cannot open file %s\n",name_file);
        exit(-1);
      }
      do
      {
        fin = fscanf(table,"%s %d ",ccode, &result);

	longueur = 0;
	while(ccode[longueur] != '\0')
	    longueur ++;
	if(longueur==1){
	  /* C'est un mot code de longueur 1 */
	  next_bit = (ccode[0]=='0' ? 0 : 1);
	  state[0].bit[next_bit] = 0;
	  state[0].result[next_bit] = result;
	  strcpy(mot[nb_entries],(next_bit ? "1" : "0"));
	  tab_code[n_code].result = result;
	  strcpy(tab_code[n_code].chain,mot[nb_entries]);
	  n_code ++;
	  continue;
	}
	for(l=0; l<longueur-1; l++){
	  /* Parcours de tous les state du state final ccode[longueur] */
	  code[l] = ccode[l];
	  code[l+1] = '\0';
	  next_bit = (ccode[l+1]=='0' ? 0 : 1);
	  for(i=1; i<nb_entries; i++)
	      if(strcmp(code,mot[i])==0){
		/* Ce mot code est connu */
		if(state[i].bit[next_bit] == -99){
		    /* Cette branche n'est pas encore initialisee */
		    if(l==longueur-2){
			/* et c'est un state final */
			state[i].bit[next_bit] = 0;
			state[i].result[next_bit] = result;
			strcpy(chain,mot[i]);
			strcat(chain,(next_bit ? "1" : "0"));
			tab_code[n_code].result = result;
			strcpy(tab_code[n_code].chain,chain);
			n_code ++;
		      }else /* et ce n'est pas un state final */
			  state[i].bit[next_bit] = nb_entries;
		}
		break;
	      }
          if(i == nb_entries){
	      /* c'est un nouveau state */
	      strcpy(mot[nb_entries], code);
	      if(l==longueur-2){
		/* C'est un state final */
		state[i].bit[next_bit] = 0;
		state[i].result[next_bit] = result;
		strcpy(chain,mot[i]);
		strcat(chain,(next_bit ? "1" : "0"));
		tab_code[n_code].result = result;
		strcpy(tab_code[n_code].chain,chain);
		n_code ++;
		nb_entries ++;
	      }else /* state n'est pas final */
		  state[i].bit[next_bit] = ++nb_entries;
	  }
	}
      } while (fin != EOF);
    fclose(table);
    /* Il reste a initialiser la racine ... */
    if(state[0].bit[0] == -99)
	for(i=1; i<nb_entries; i++)
	    if(mot[i][0] == '0'){
	      /* mot[i] est le 1er mot de l'arbre commencant par 0 */
	      state[0].bit[0] = i;
	      break;
            }
    if(state[0].bit[1] == -99)
	for(i=1; i<nb_entries; i++)
	    if(mot[i][0] == '1'){
	      /* mot[i] est le 1er mot de l'arbre commencant par 1 */
	      state[0].bit[1] = i;
	      break;
            }
    /* Si besoin est, proceder ici a un tri de tab_code.
    *  Dans le cas general, le fichier d'entree qui contient les
    *  codes de Huffman est deja trie.
    */
/*
    fprintf(stderr,"\n\t\t\t*** FILE %s ***\n",name_file);
    for(l=0; l<nb_entries; l++)
	fprintf(stderr,"state[%3d]= %s *** 0-> %d, 1-> %d ** 0: %d, 1: %d\n",
                 l,mot[l],state[l].bit[0],state[l].bit[1],state[l].result[0],
                 state[l].result[1]);
    printf("state de %s a %d elements\n",name_file, nb_entries);
*/
    return(n_code);
  }



main()
{
  int n_adpcm;
  static etat state_adpcm[L_STATE];
  static mot_code tab_adpcm[L_STATE];
  register int i;

  n_adpcm = create_vlc("VLC.ADPCM", state_adpcm, tab_adpcm);
  printf("\n\nstatic int n_adpcm = %d;\n\n", n_adpcm);
  printf("static etat state_adpcm[] = {\n\t");
  for(i=0; i<n_adpcm; i++){
    printf("{{%d,%d},{%d,%d}}, ", state_adpcm[i].bit[0],
	   state_adpcm[i].bit[1], state_adpcm[i].result[0],
	   state_adpcm[i].result[1]);
    if(((i+1)%3)==0)
      printf("\n\t");
  }
  printf("\n};\n\n\n\n");
  printf("static mot_code tab_adpcm[] = {\n\t");
  for(i=0; i<n_adpcm; i++){
    printf("{%d, \"%s\"}, ", tab_adpcm[i].result, tab_adpcm[i].chain);
    if(((i+1)%3)==0)
      printf("\n\t");
  }
  printf("\n};\n\n");
}
      

	

	    
	


