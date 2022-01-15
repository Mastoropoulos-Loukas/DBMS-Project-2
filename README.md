# Μέλη ομάδας
Λινάρδος Αλέξανδρος - 1115201600093

Μαστορόπουλος Λουκάς - 1115201700078

Χαρίσης Νικόλαος - 1115201700187

# Οδηγίες μεταγλώττισης και εκτέλεσης
* Για τη μεταγλώττιση του προγράμματος χρησιμοποιήστε την εντολή `make sht`
* Για την εκτέλεση χρησιμοποιήστε την εντολή `./build/runner`
* Για την διαγραφή των αρχείων .db και των εκτελέσιμων χρησιμοποιήστε την `make clean`

# Παραδοχές
* Έχουν υλοποιηθεί όλες οι ζητούμενες συναρτήσεις και λειτουργίες, παρολ' αυτά ο αριθμός των εγγραφών που μπορούν να εισαχθούν είναι περιορισμένος. Πιο συγκεκριμένα, στην τωρινή υλοποίηση του προγράμματος μπορούν να εισαχθούν σωστά μέχρι και 90 εγγραφές. Ο αριθμός αυτός ενδέχεται να διαφοροποιείται ανάλογα με το seed της srand() που ορίζεται στην main, λόγω του ότι η rand μπορεί να επιλέγει κάποια στοιχεία με μεγάλη συχνότητα. Επιπλέον, τα ευρετήρια περιορίζονται σε ένα μόνο μπλοκ. Ο αριθμός αυτός θα μπορούσε να είναι μεγαλύτερος αν οι πίνακες cities και surnames είχανε περισσότερα διαφορετικά στοιχεία έτσι ώστε να μην επιλέγονται τόσο συχνά ίδιες εγγραφές.
* Πήραμε την απόφαση να "σπάσουμε" τον κώδικα σε πολλές μικρότερες συναρτήσεις προκειμένου να είναι οι ζητούμενες συναρτήσεις πιο ευανάγνωστες. Στη συνέχεια ακολουθεί κατάλογος των εν λόγω συναρτήσεων.

# Κατάλογος συναρτήσεων
Για πιο αναλυτικές πληροφορίες σχετικά με κάθε συνάρτηση και τις παραμέτρους της έχουμε αφήσει επαρκή σχόλια πάνω στον κώδικα.

## Βοηθητικές συναρτήσεις ανά αρχείο
* __hash_file.c__:
    * checkCreateIndex
    * checkInsertEntry
    * checkPrintAllEntries
    * getTid
    * getDepth
    * getHashTable
    * getBucket
    * getEntry
    * getEndPoints
    * getNewBlock
    * getBlockNumFromTID
    * getIndexFromTID
    * setDepth
    * setHashTable
    * setEntry
    * createInfoBlock
    * createHashTable
    * reassignRecords
    * insertRecordAfterSplit
    * doubleHashTable
    * splitHashTable
    * printUpdateArray
    * printRecord
    * printHashNode
    * printAllRecords
    * printSepsificRecord
    * hashFunction

* __sht_file.c__:
    * checkShtCreate
    * checkSecInsertEntry
    * getSecHashTable
    * getSecBucket
    * getSecEntry
    * getSecEndPoints
    * setSecHashTable
    * setSecEntry
    * createSecInfoBlock
    * createSecHashTable
    * reassignSecRecords
    * insertSecRecordAfterSplit
    * doubleSecHashTable
    * splitSecHashTable
    * printSecRecord
    * SHT_PrintHashNode
    * SHT_PrintSecondaryRecord
    * SHT_PrintSecEntry
    * SHT_PrintSecHashEntry
    * SHT_PrintSecHashTable
    * printAllSecRecords
    * printSecSepsificRecord
    * primaryExists
    * hashAttr

## Ζητούμενες συναρτήσεις (στο αρχέιο sht_file.c)
* SHT_Init
* SHT_CreateSecondaryIndex
* SHT_CloseSecondaryIndex
* SHT_SecondaryInsertEntry
* SHT_SecondaryUpdateEntry
* SHT_PrintAllEntries
* SHT_HashStatistics
* SHT_InnerJoin