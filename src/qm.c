#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <omp.h>

#define MAX_VARS 13                             // Use just one more or many more than needed for correct results
#define MAX_SETS 1000000                        // Increase if segfault

#define ENABLE_MP 1                             // Enable or disable Multi-Threading
#define NUM_THREADS 12                          // Set number of threads

#define NEW_LINE(num) \
    for (int i = 0; i < (num); i++) \
        printf("\n")

#define SPACES(num) \
    for (int i = 0; i < (num); i++) \
        printf(" ")

#define PRINT(num, char) \
    for (int i = 0; i < (num); i++) \
        printf("%c", char)

#define ARR_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
    int *minterms;
    int  num_minterms;
    char binary[MAX_VARS];
    int  isPrimeImplicant;
} MintermSet;

typedef struct {
    char *expr;
} Expression;

int NUM_GROUP_PRINTS = 0;
int NUM_TABLE_PRINTS = 0;

MintermSet *createMintermSet() {
    MintermSet *newList = malloc(sizeof(MintermSet));
    if (newList == NULL) {   
        printf("Error creating MintermSet\n");
        return NULL;
    }

    newList->minterms = NULL;
    newList->num_minterms = 0;
    newList->isPrimeImplicant = 0;
    newList->binary[0] = '\0';

    return newList;
}

void deleteMintermSets(MintermSet** lists, int num_lists) {
    if(lists == NULL)
        return;
    for (int i = 0; i < num_lists; i++) {
        if (lists[i] != NULL) {
            if(lists[i]->minterms != NULL) {
                free(lists[i]->minterms);
                lists[i]->minterms = NULL;
            }
            free(lists[i]);
            lists[i] = NULL;
        }
    }
    if(lists != NULL) {
        free(lists);
        lists = NULL;
    }
}

void deleteMintermSet(MintermSet* list) {
    if(list == NULL)
        return;
    if(list->minterms != NULL) {
        free(list->minterms);
        list->minterms = NULL;
    }   
    free(list);
    list = NULL;
}

void deleteExpression(Expression *exprs, int num_exprs) {
    for(int i = 0; i < num_exprs; i++) {
        if(exprs[i].expr != NULL) {
            free(exprs[i].expr);
            exprs[i].expr = NULL;
        }
    }
    free(exprs);
    exprs = NULL;
}

void appendExpression(Expression **list, int *num_list, char *exprToAppend) {
    // Increase size
    (*num_list)++;
    Expression *temp = realloc(*list, (*num_list) *sizeof(Expression));
    if(temp == NULL) {
        printf("realloc failed at appendExpression\n");
        free(temp);
        return;
    }
    (*list) = temp;

    // Append new expression
    (*list)[(*num_list) - 1].expr = malloc((strlen(exprToAppend) + 1) * sizeof(char));
    strcpy((*list)[(*num_list) - 1].expr, exprToAppend);
}    

void appendMinterm(MintermSet *list, int minterm) {
    // Duplicate check
    for (int i = 0; i < list->num_minterms; i++) {
        if (list->minterms[i] == minterm) {
            // Duplicate found, reject it
            return;
        }
    }

    // Increase size
    int *temp = NULL;
    temp = realloc(list->minterms, (list->num_minterms + 1) * sizeof(int));
    if (temp == NULL) {
        printf("realloc failed at appendMinterm\n");
        return;
    }
    temp[list->num_minterms] = minterm;
    
    // Append the new minterm
    list->minterms = temp;
    list->num_minterms++;
}

void removeMinterm(int *minterms, int *num_minterms, int mintermToRemove) {
    int i;
    int indexToRemove = -1;

    for(i = 0; i < (*num_minterms); i++) {
        if(minterms[i] == mintermToRemove) {
            indexToRemove = i;
            break;
        }
    }

    if(indexToRemove != -1) {
        for(i = indexToRemove; i < (*num_minterms) - 1; i++) {
            minterms[i] = minterms[i+1];
        }
        (*num_minterms)--;
    }
}

void copyMintermSet(MintermSet *list1, MintermSet *list2) {
    int i;

    // Allocate memory for the minterms
    list1->minterms = malloc(list2->num_minterms * sizeof(*(list1->minterms)));
    if (list1->minterms == NULL) {
        printf("Memory allocation error in copyMintermSet\n");
        return;
    }
    // Copy the minterms
    for (i = 0; i < list2->num_minterms; i++) {
        list1->minterms[i] = list2->minterms[i];
    }

    // Copy the rest
    list1->num_minterms = list2->num_minterms;
    list1->isPrimeImplicant = list2->isPrimeImplicant;
    strcpy(list1->binary, list2->binary);
}

void print_groups(MintermSet **groups, int num_groups) {
    int i, j;

    printf("\n\x1B[33mGroup %d:\033[0m\n", NUM_GROUP_PRINTS++);
    for (i = 0; i < num_groups; i++) {
        printf("    Set%2d:  ", i);
        MintermSet *group = groups[i];
        printf("%3s| ",group->binary);
        for (j = 0; j < group->num_minterms; j++) {
            printf("%3d ", group->minterms[j]);
        }
        printf("|");
        if(group->isPrimeImplicant) {
            printf("\033[0;32m%3d\033[0;0m\n", group->isPrimeImplicant);
        }
        else {
            printf("%3d\n", group->isPrimeImplicant);
        }
    }
}

void printPrimeImplicantTable(MintermSet **primeImps, int num_primeImps, 
                              int *uniqueMints, int num_uniqueMints) {
    int i, j, k;
    int maxMintLen = 0;

    printf("\n\x1B[33mTable state %d:\033[0m\n", NUM_TABLE_PRINTS++);
    // Find length of the biggest minterm for spacing 
    for (i = 0; i < num_uniqueMints; i++) {
        int currMintLen = 1;
        int num = uniqueMints[i];
        while (num /= 10) {
            currMintLen++;
        }
        if (currMintLen > maxMintLen) {
            maxMintLen = currMintLen;
        }
    }

    int binarySpacing = MAX_VARS;
    int mintSpacing = maxMintLen + 1;
    // Print the minterms
    SPACES(1);
    printf("%-*s", binarySpacing,"");
    for (i = 0; i < num_uniqueMints; i++) {
        printf("%-*d  ", mintSpacing, uniqueMints[i]);
    }
    NEW_LINE(1);

    // Print the table rows
    for (i = 0; i < num_primeImps; i++) {
        MintermSet *primeImp = primeImps[i];
        printf("\033[0;34m%-*s\033[1;0m", binarySpacing, primeImp->binary);
        for (j = 0; j < num_uniqueMints; j++) {
            int found = 0;
            for (k = 0; k < primeImp->num_minterms; k++) {
                if (primeImp->minterms[k] == uniqueMints[j]) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                printf("  \033[0;32mX\033[1;0m |");
            } else {
                printf("  - |");
            }
        }
        NEW_LINE(1);
    }
}

void printExpression(Expression *list, int num_list) {
    int i;

    NEW_LINE(1);
    printf("\033[0;31mF = ");
    for(i = 0; i < num_list; i++) {
        if(i != num_list - 1) {
            printf("%s + ", list[i].expr);
        }
        else {
            printf("%s", list[i].expr);
        }
    }
    printf("\033[0;0m");
    NEW_LINE(1);
}

int bit_diff(const char *first_binary, const char *second_binary) {
    int i;
    int num_diff_bits = 0;
    int different_bit = 0;
    int LEN = MAX_VARS - 1;

    for(i = 0; i < LEN; i++) {
        if(first_binary[i] != second_binary[i]) {
            num_diff_bits++;
            different_bit = i;

            if(num_diff_bits > 1) {
                return 0;
            }
        }
    }

    if(num_diff_bits == 1) {
        return different_bit;
    }
    else {
        return 0;
    }
}

void intToBinary(int number, char binary[MAX_VARS]) {
    int i, d;
    int pos = MAX_VARS - 1;

    // Fill binary with 0's
    for (i = 0; i < MAX_VARS; i++) {
        binary[i] = '0';
    }

    // NULL terminate bin
    binary[pos] = '\0';
    pos--;

    // Calculate binary with common div method
    while (number > 0) {
        d = (number % 2);
        if (pos < 0){
            break;
        }
        binary[pos--] = d + '0';
        number = (number - d) / 2;
    }
}

void convertBinaryToExpression(char *binary, int num_variables) {
    int i;
    int LEN = strlen(binary);
    // Negation will be also appended so size hasto incease to account it plus \0
    char newBinary[num_variables * 2 + 1];  
    int num_newBinary = 0;
    newBinary[0] = '\0';

    const char uppercaseLetters[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
    int num_letters = 0;

    int start = LEN - num_variables;
    for(i = start; i < LEN; i++) {
        if(binary[i] == '-') {
            num_letters++;
            continue;
        }
        if(binary[i] == '0') {
            newBinary[num_newBinary++] = uppercaseLetters[num_letters++];
            newBinary[num_newBinary++] = '\'';
        }
        else if(binary[i] == '1') {
            newBinary[num_newBinary++] = uppercaseLetters[num_letters++];
        }
    }
    newBinary[num_newBinary] = '\0';
    strcpy(binary, newBinary);
}

void markPrimeImplicants(MintermSet **groups, int num_groups, 
                         MintermSet **new_groups, int num_new_groups) {
    int i, j;

    #if ENABLE_MP
        #pragma omp parallel for private(j) schedule(dynamic)
    #endif
    for (i = 0; i < num_groups; i++) {
        int isPrime = 1;

        for (j = 0; j < num_new_groups; j++) {
            if (bit_diff(groups[i]->binary, new_groups[j]->binary)) {
                isPrime = 0;
                break;
            }
        }
        groups[i]->isPrimeImplicant = isPrime;
    }
}

void extractUniqueMinterms(MintermSet **groups, int num_groups, 
                           int *uniqueMints, int *num_uniqueMints) {
    int i,j,k;

    for(i = 0; i < num_groups; i++) {
        MintermSet *group = groups[i];
        for(j = 0; j < group->num_minterms; j++) {
            int isUnique = 1;
            
            // Check if currentMint already exists
            for(k = 0; k < (*num_uniqueMints); k++) {
                if(uniqueMints[k] == group->minterms[j]) {
                    isUnique = 0;
                }
            }

            // If the current Mint is unique, append it
            if(isUnique) {
                uniqueMints[(*num_uniqueMints)] = group->minterms[j];
                (*num_uniqueMints)++;
            }
        }
    }
}

void bubbleSort(int *arr, int num_arr) {
    int i, j;
    int temp;

    for (i = 0; i < num_arr; i++) {
        for (j = 0; j < (num_arr - 1); j++) {
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int compareBinary(const void* p, const void* q) {
    MintermSet* left =  *((MintermSet**)p);
    MintermSet* right = *((MintermSet**)q);
    return strcmp(left->binary, right->binary);
}

void mergeArrays(int *minterms, int num_minterms, int *donts, int num_donts, 
                 int *combined_minterms) {
    int i = 0, j = 0, k = 0;

    while(i < num_minterms) {
        combined_minterms[k++] = minterms[i++];
    }
    while(j < num_donts) {
        combined_minterms[k++] = donts[j++];
    }

    bubbleSort(combined_minterms, (num_minterms + num_donts));
}

void removeDontCares(int *minterms, int *num_minterms, int *donts, int num_donts) {
    int i,j,k;

    for(i = 0; i < (*num_minterms); i++) {
        for(j = 0; j < num_donts; j++) {

            if(minterms[i] == donts[j]) {
                for(k = i; k < (*num_minterms); k++) {
                    minterms[k] = minterms[k + 1];
                }
                (*num_minterms)--;
            }
        }
    }
}

int column_dominance(MintermSet **primeTable, int *num_primeTable, Expression **result, int *num_result, 
                      int *uniqueMints, int *num_uniqueMints, int printEnabled) {
    int i, j, k, z;
    int can_remove = 0;
    
    for (i = 0; i < (*num_uniqueMints); i++) {
        int essential = 0;
        int implicant_pos = 0;

        for(j = 0; j < (*num_primeTable); j++) {
            for(k = 0; k < primeTable[j]->num_minterms; k++) {
                if(primeTable[j]->minterms[k] == uniqueMints[i]) {
                    if(++essential > 1)
                        break;
                    implicant_pos = j;
                }  
            }
            if(essential > 1)
                break;
        }
        
        if (essential == 1) {
            can_remove = 1;
            appendExpression(result, num_result, primeTable[implicant_pos]->binary);

            if(printEnabled){
                printf("prime implicant \033[0;31m%s\033[1;0m is essential\n", primeTable[implicant_pos]->binary);
                printf("remove minterms ");
            }  

            for (z = 0; z < primeTable[implicant_pos]->num_minterms; z++) {
                int minterm = primeTable[implicant_pos]->minterms[z];
                if(printEnabled){
                    printf("%d ", minterm);
                }
                removeMinterm(uniqueMints, num_uniqueMints, minterm);
            }
            // Remove the current prime implicant from primeTable
            deleteMintermSet(primeTable[implicant_pos]);
            for (k = implicant_pos; k < (*num_primeTable)-1; k++) {
                primeTable[k] = primeTable[k+1];
            }
            (*num_primeTable)--;
            if(printEnabled) {
                printf("with column dominance\n");
                printPrimeImplicantTable(primeTable, (*num_primeTable), 
                    uniqueMints, (*num_uniqueMints));
                NEW_LINE(1);
            }
            break;
        }
    }
    
    return can_remove;
}

int row_dominance(MintermSet **primeTable, int *num_primeTable,  Expression **result, int *num_result, 
                   int *uniqueMints, int *num_uniqueMints, int printEnabled) {
    int i, j, k;
    int can_remove = 0;

    
    for (i = 0; i < (*num_primeTable); i++) {
        MintermSet *row = primeTable[i];
        int essential = 0;

        for (k = 0; k < row->num_minterms; k++) {
            for (j = 0; j < (*num_uniqueMints); j++) {
                if (row->minterms[k] == uniqueMints[j]) {
                    essential++;
                    break;
                }
            }
        }

        if(essential == 1){
            can_remove = 1;
            appendExpression(result, num_result, row->binary);
            if(printEnabled) {
                printf("prime implicant \033[0;31m%s\033[1;0m is essential\n", row->binary);
                printf("remove minterms ");
            }

            for(k = 0; k < row->num_minterms; k++){
                if(printEnabled) {
                    printf("%d ",row->minterms[k]);
                }
                removeMinterm(uniqueMints, num_uniqueMints, row->minterms[k]);
            }
            if(printEnabled) {
                printf("with row dominance\n");
                printPrimeImplicantTable(primeTable, (*num_primeTable), 
                    uniqueMints, (*num_uniqueMints));
                NEW_LINE(1);
            }
            
            deleteMintermSet(row);
            // Remove the current prime implicant from primeTable
            for(j = i; j < (*num_primeTable) - 1; j++) {
                primeTable[j] = primeTable[j+1];
            }
            (*num_primeTable)--;
            break;
        }
    }
    return can_remove;
}

int merge_minterms(MintermSet ***groups, int *num_groups, 
                   MintermSet ***new_groups, int *num_new_groups, 
                   MintermSet ***primeImps, int *num_primeImps, int printEnabled) {

    int i, j, k;
    int bit_pos;
    int cant_merge = 1;
    double run_time, start_time;
    // Creating a local counter for groups made a big difference in speed
    int local_num_groups = (*num_groups);
    char temp_binary[MAX_VARS];
    
    start_time = omp_get_wtime();
    // In this case schedule(dynamic) gave best results 
    #if ENABLE_MP
        #pragma omp parallel for private(j, k, bit_pos, temp_binary) shared(cant_merge) schedule(dynamic)
    #endif
    for (i = 0; i < local_num_groups - 1; i++) {
        for (j = i + 1; j < local_num_groups; j++) {
            bit_pos = bit_diff((*groups)[i]->binary, (*groups)[j]->binary);

            if (bit_pos != 0) {
                cant_merge = 0; // Signals that there are still groups to be merged
                int isDuplicate = 0;
                int local_num_ngroups = (*num_new_groups);

                strcpy(temp_binary, (*groups)[i]->binary);
                temp_binary[bit_pos] = '-';
                
                
                // Check if it is duplicate
                for (k = 0; k < local_num_ngroups; k++) {
                    char *first_bin = (*new_groups)[k]->binary;  
                    if (strcmp(first_bin, temp_binary) == 0) {
                        isDuplicate = 1;
                        break;
                    }
                }
                
               
                if (!isDuplicate) {
                    MintermSet *merged_set = createMintermSet();

                    for (k = 0; k < (*groups)[i]->num_minterms; k++) {
                        appendMinterm(merged_set, (*groups)[i]->minterms[k]);
                    }
                    for (k = 0; k < (*groups)[j]->num_minterms; k++) {
                        appendMinterm(merged_set, (*groups)[j]->minterms[k]);
                    }
                    strcpy(merged_set->binary, temp_binary);
                    
                    #if ENABLE_MP
                        #pragma omp critical
                        {
                            //deleteMintermSet((*new_groups)[(*num_new_groups)]); // Segfault
                            (*new_groups)[(*num_new_groups)++] = merged_set;
                        }
                    #else
                        deleteMintermSet((*new_groups)[(*num_new_groups)]);
                        (*new_groups)[(*num_new_groups)++] = merged_set;
                    #endif
                }
            }
        }
    }

    #if ENABLE_MP
        int local_num_ngroups = (*num_new_groups);
        int duplicate_found = 0;

        // Sort comparing the binaries so duplicates will be next to each other 
        qsort((*new_groups), (local_num_ngroups), sizeof(MintermSet *), compareBinary);

        // Remove duplicates from the new group
        for( i = 0; i < local_num_ngroups - 1; i++) {
            duplicate_found = strcmp((*new_groups)[i]->binary, (*new_groups)[i+1]->binary);
            if(!duplicate_found) {
                // Shift elements
                for(k = i; k < local_num_ngroups - 1; k++) 
                    (*new_groups)[k] = (*new_groups)[k+1];
                (*num_new_groups)--;
                local_num_ngroups--;
                i--;  
            }
        }

    #endif

    // Find and append prime implicants
    markPrimeImplicants((*groups), (*num_groups), (*new_groups), (*num_new_groups));
    for (int i = 0; i < local_num_groups; i++) {
        if ((*groups)[i]->isPrimeImplicant) {
            MintermSet *primeImplicant = createMintermSet();
            copyMintermSet(primeImplicant, (*groups)[i]);
            (*primeImps)[(*num_primeImps)] = primeImplicant;
            (*num_primeImps)++;
        }
    }
    
    run_time = omp_get_wtime();
    
    if(printEnabled) {
        print_groups((*groups), (*num_groups));
    } else {
        printf("--Merging %5d sets took %.2fs...\n", (*num_groups), (run_time - start_time));
    }

    // Swap pointers to swap groups with new_groups for the next grouping
    MintermSet **temp = (*groups);
    (*groups) = (*new_groups);
    (*new_groups) = temp;

    // Swap their counters to reflect the change
    (*num_groups) = (*num_new_groups); 
    (*num_new_groups) = 0;
    return cant_merge;
}

void mcluskey(int *minterms, int num_minterms, int num_variables, 
              int *donts, int num_donts, int printEnabled) {
    int i, j;
    int done = 0;
    double start_time, run_time;
    int col_done = 0;
    int row_done = 0;
    int num_uniqueMints = 0;
    int uniqueMints[num_minterms];
    int num_combined_minterms = 0;
    int *combined_minterms;
    int num_groups = 0;
    int num_new_groups = 0;
    int num_primeImps = 0;
    MintermSet **groups =     malloc(MAX_SETS * sizeof(MintermSet *));
    MintermSet **new_groups = malloc(MAX_SETS * sizeof(MintermSet *));
    MintermSet **primeImps =  malloc(MAX_SETS * sizeof(MintermSet *));
    int num_result = 0;
    Expression *result = NULL;

    if (ENABLE_MP) {
        omp_set_num_threads(NUM_THREADS);
        PRINT(60, '=');
        NEW_LINE(1);
        printf("OpenMP enabled\n");
    } else {
        PRINT(60, '=');
        NEW_LINE(1);
        printf("OpenMP disabled\n");
    }

    start_time = omp_get_wtime();
    for (i = 0; i < MAX_SETS; i++) {
        groups[i] =     NULL;
        new_groups[i] = NULL;
        primeImps[i] =  NULL;
    }

    // Check if there are don't cares and insert accordingly
    num_combined_minterms = num_minterms + num_donts;
    combined_minterms = malloc((num_combined_minterms) * sizeof(*combined_minterms));
    if (donts != NULL) {
        mergeArrays(minterms, num_minterms, donts, num_donts, 
                    combined_minterms);
    } else {
        for(i = 0; i < num_minterms; i ++) {
            combined_minterms[i] = minterms[i];
        }
    }

    for (j = 0; j < num_combined_minterms; j++) {
        groups[j] = createMintermSet();
        appendMinterm(groups[j], combined_minterms[j]);
        num_groups++;
        intToBinary(combined_minterms[j], groups[j]->binary);
    }
    run_time = omp_get_wtime();

    if(!printEnabled)
        printf("Initializing minterms took %.2fs...\n", (run_time - start_time));


    start_time = omp_get_wtime();
    // Grouping loop 
    while (!done) {
        done = merge_minterms(&groups, &num_groups, 
            &new_groups, &num_new_groups, &primeImps, &num_primeImps, printEnabled);
    }
    free(combined_minterms);
    run_time = omp_get_wtime();

    if(!printEnabled) {
        printf("Finding prime implicants took %.2fs...", (run_time - start_time));
        NEW_LINE(1);
    }

    memset(uniqueMints, 0, num_minterms * sizeof(*uniqueMints));
    extractUniqueMinterms(primeImps, num_primeImps, uniqueMints, &num_uniqueMints);
    bubbleSort(uniqueMints, num_uniqueMints);

    if (donts != NULL) {
        removeDontCares(uniqueMints, &num_uniqueMints, donts, num_donts);
    }

    for (i = 0; i < num_primeImps; i++) {
        convertBinaryToExpression(primeImps[i]->binary, num_variables);
    }

    if(printEnabled) {
        printPrimeImplicantTable(primeImps, num_primeImps, 
            uniqueMints, num_uniqueMints);
    }

    start_time = omp_get_wtime();
    // While there are still minterms find essential prime implicants
    col_done = 1;
    row_done = 1;
    while(num_uniqueMints) {
        col_done = 1;
        while(col_done) {
            col_done = column_dominance(primeImps, &num_primeImps, &result, &num_result, 
                            uniqueMints, &num_uniqueMints, printEnabled);
        }
        row_done = row_dominance(primeImps, &num_primeImps, &result, &num_result,
            uniqueMints, &num_uniqueMints, printEnabled);
        
        if((row_done + col_done) == 0){
            // If the table is stuck remove the first prime implicant
            if(num_uniqueMints) {
                appendExpression(&result, &num_result, primeImps[0]->binary);
                if(printEnabled) {
                    printf("Column, Row dominance stuck removing first implicant \033[0;31m%s\033[1;0m\n", 
                        primeImps[0]->binary);
                    printf("remove minterms ");
                }
                for(i = 0; i < primeImps[0]->num_minterms; i++){
                    if(printEnabled) {
                        printf("%d ",primeImps[0]->minterms[i]);
                    }
                    removeMinterm(uniqueMints, &num_uniqueMints, primeImps[0]->minterms[i]);
                }
                NEW_LINE(1);
                deleteMintermSet(primeImps[0]);
                for (i = 0; i < num_primeImps-1; i++) {
                    primeImps[i] = primeImps[i+1];
                }
                num_primeImps--;
            }
            else
                break;
        }
    }

    run_time = omp_get_wtime();
    if(!printEnabled)
        printf("Finding essential implicants took %.2fs...\n", (run_time - start_time));
    else{
        printExpression(result, num_result);
    }
    PRINT(60, '=');
    NEW_LINE(1);
    
    // Reset print counters
    NUM_GROUP_PRINTS = 0;
    NUM_TABLE_PRINTS = 0;
    // Free memory used by MintermSets and Expression
    deleteExpression(result, num_result);
    #if(!ENABLE_MP)
        deleteMintermSets(groups, MAX_SETS);        // When OpenMP=enabled this causes a segfault 
        deleteMintermSets(new_groups, MAX_SETS);    // When OpenMP=enabled this causes a segfault
    #endif
    deleteMintermSets(primeImps, num_primeImps);  
}

int main(int argc, char **argv) {

    if(!strcmp(argv[1],"0")) {
        // Example without don't cares
        char input[1024];
        int minterms[1024];
        int num_minterms = 0; 
        int num_variables = 0;
        memset(input, '\0', 1024);
        printf("Give minterms:");
        fgets(input, sizeof(input), stdin); fflush(stdin);
        char* token = strtok(input, " ");

        while (token != NULL) {
            minterms[num_minterms++] = atoi(token);
            token = strtok(NULL, " ");
        }
        printf("How many variables:");
        scanf("%d", &num_variables);    
        mcluskey(minterms, num_minterms, num_variables, NULL, 0, 1);
    }

    // Demo 1
    if(!strcmp(argv[1],"1")) {
        // Example without don't cares
        int minterms[] = {0, 1, 2, 5, 7, 8, 9, 10, 13, 15};
        int num_minterms = ARR_LEN(minterms); 
        int num_variables = 4; // Must be exactly enough to accomodate the biggest minterm
        mcluskey(minterms, num_minterms, num_variables, NULL, 0, 1);
    }

    // Demo 2
    if(!strcmp(argv[1],"2")) {
        // Example with don't cares
        int minterms[] = {2, 4, 6, 8, 10, 12, 32, 128};
        int donts[] = {3, 5};
        int num_minterms = ARR_LEN(minterms); 
        int num_donts = ARR_LEN(donts);
        int num_variables = 7; // 2^7 = 128 = our biggest minterm
        mcluskey(minterms, num_minterms, num_variables, donts, num_donts, 1); 
    }

    // Demo 3
    if(!strcmp(argv[1],"3")) {
        // Example of stressing the code 
        srand(time(NULL));
        int num_minterms = 0;
        int minterms[64];
        for(int i = 0; i < 64; i++){
            if(rand() % 2)
                minterms[num_minterms++] = i;
        }
        //bubbleSort(minterms, num_minterms);
        int num_variables = 6;
        mcluskey(minterms, num_minterms, num_variables, NULL, 0, 1);
    }

    // Demo 4
    if(!strcmp(argv[1],"4")) {
        // Example of stressing the code 
        int num_minterms = 4096;
        int minterms[num_minterms];
        for(int i = 0; i < num_minterms; i++){
            minterms[i] = i;
        }
        int num_variables = 12;
        mcluskey(minterms, num_minterms, num_variables, NULL, 0, 0);
    }

    return 0;
}