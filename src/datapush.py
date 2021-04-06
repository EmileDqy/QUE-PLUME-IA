# -*- coding: utf-8 -*-
"""
Created on Tue Apr  6 11:16:40 2021

@author: Lucie
"""
import os
try:
  import pyrebase
except ImportError:
  os.system('python3 -m pip install pyrebase')
import pyrebase
import argparse
import datetime

config = {
  "apiKey": "AAAA_8U_Wc0:APA91bHGpXr9EqK0gsk25OkcdO5cVV3I7b83dj0dIS4ri1X__HxC86UhRXnHeTbdwZ6BINuLwJ7zf0lnDaau4toJ7EitnMFnna1ZaXGJWivVg3w3H7_d8zwe7UwWBlqrXvLZiEuX5zPl",
  "authDomain": "projetia-d96bc.firebaseapp.com",
  "databaseURL": "https://projetia-d96bc-default-rtdb.firebaseio.com",
  "storageBucket": "projetia-d96bc.appspot.com"
}

if __name__ == "__main__":
  #argparser
  my_parser = argparse.ArgumentParser(description='Push data to firebase')
  my_parser.add_argument('type_expected',
                        help='type expected of the object')
  my_parser.add_argument('color_expected',
                        help='color expected of the object')
  my_parser.add_argument('type_found',
                        help='type estimated for the object')
  my_parser.add_argument('color_found',
                        help='color estimated for the object')

  # Setup firebase
  firebase = pyrebase.initialize_app(config)
  db = firebase.database()
  
  # Vars
  dateheure = str(datetime.datetime.now())

  # Execute parse_args()
  args = my_parser.parse_args()
  data = {"type_reel":args.type_expected,"couleur_reelle":args.color_expected,"date":dateheure,"couleur_trouvee" : args.color_found,"type_trouve" :args.type_found}
  db.child("traites").push(data)
  type_reel = data["type_reel"]
  couleur_reelle = data["couleur_reelle"]
  type_trouve = data["type_trouve"]
  couleur_trouvee = data["couleur_trouvee"]
  if (type_reel == type_trouve and couleur_reelle == couleur_trouvee) :
      if (type_reel == "couvercle" or couleur_reelle == "rouge") :
          bon = db.child("resultat").child("true_" + type_reel + "_" + couleur_reelle).get().val()
          db.child("resultat").update({"true_" + type_reel + "_" + couleur_reelle : bon+1})
      else :
        if couleur_reelle == "blanc" :
            bon = db.child("resultat").child("true_" + type_reel + "_" + couleur_reelle + "he").get().val()
            db.child("resultat").update({"true_" + type_reel + "_" + couleur_reelle + "he": bon+1})
        else :
                bon = db.child("resultat").child("true_" + type_reel + "_" + couleur_reelle + "e").get().val()
                db.child("resultat").update({"true_" + type_reel + "_" + couleur_reelle + "e": bon+1})
  else :
        if (type_reel == "couvercle" or couleur_reelle =="rouge") :
            faux = db.child("resultat").child("false_" + type_reel + "_" + couleur_reelle).get().val()
            db.child("resultat").update({"false_" + type_reel + "_" + couleur_reelle : faux+1})
        else :
            if couleur_reelle == "blanc" :
                faux = db.child("resultat").child("false_" + type_reel + "_" + couleur_reelle + "he").get().val()
                db.child("resultat").update({"false_" + type_reel + "_" + couleur_reelle + "he" : faux+1})
            else :
                faux = db.child("resultat").child("false_" + type_reel + "_" + couleur_reelle + "e").get().val()
                db.child("resultat").update({"false_" + type_reel + "_" + couleur_reelle + "e" : faux+1})
  print('The data has been pushed')




