//Include libraries
#include "digger.h"
#include <immintrin.h>
#include <omp.h>

//CREATE THE SEED ELEMENTS TO FORM THE TREE
void seed()
{
    int *subset,*column;
    int i,j;
    gettimeofday(&start,NULL);
    
    //RECORDS TO BUILD THE ROOT ELEMENT
    subset = (int *) malloc((g_line_count) * sizeof(int));
    for (i=0;i<g_line_count;i++)
    {
        subset[i]=i;
    }
    //TOTAL COLUMNS PARTICIPATING IN THE DECISION
    column = (int *) malloc((g_column_count) * sizeof(int));
    j=0;
    for (i=0;i<g_column_count;i++)
    {
        if (g_target==i)
        {
            continue;
        }
        column[j]=i;
        j++;
    }
    
    op = fopen("Rules", "w");
    if (op == NULL)
    {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(op,"Target attribute is: %s\n",g_headers[g_target]);
    root = (node *)malloc(sizeof(node));
    root->item_count=0;
    //INITIAL CALL TO START THE TREE
    grow(subset,column,g_line_count,g_column_count-1,root);
    printf("Generated Tree is stored in file Rules\n");
    gettimeofday(&end,NULL);
    fclose(op);
    free(subset);
    free(column);
}

//INCREMENTAL RECURSIVE BUILDING BLOCK TO BUILD TREE
int grow(int *ip_subset,int *ip_column,int ip_line_count,int ip_column_count,node *ip_node)
{
    //STATIC VARIABLE TO TRACK THE CURRENT LEVEL IN THE TREE
    __m256i vec[ip_column_count];
    __m256i vec1[ip_column_count];
    __m256i vec2[ip_column_count];
    __m256i vcmp1[ip_column_count];
    __m256i vcmp2[ip_column_count];
    int i;
    int ***subset, **item, *item_count, **item_line_count, ***item_target, **item_target_count, ***item_target_line_count;
    int *column, **n_subset, split_column, *n_item, *n_item_line_count, **n_item_target, *n_item_target_count, **n_item_target_line_count; //non-parallel
    int n_item_count = 0; //non-parallel
    float *gain, *item_gain;
    float n_gain, max_prob; //non-parallel
    int max_target, item_flag; //non-parallel
    n_gain = -1.0; //non-parallel
    
    subset = (int ***) malloc(ip_column_count * sizeof(int **));
    item = (int **) malloc(ip_column_count * sizeof(int *));
    item_count = (int *) malloc(ip_column_count * sizeof(int));
    item_line_count = (int **) malloc(ip_column_count * sizeof(int *));
    item_target = (int ***) malloc(ip_column_count * sizeof(int **));
    item_target_count = (int **) malloc(ip_column_count * sizeof(int *));
    item_target_line_count = (int ***) malloc(ip_column_count * sizeof(int **));
    gain = (float *) malloc(ip_column_count * sizeof(float));
    item_gain = (float *) malloc(ip_column_count * sizeof(float));
    
    //LOOP ATTRIBUTES
#pragma omp parallel for num_threads(16)
    for (i=0; i<ip_column_count; i++)
    {
        int j, k, l;
        item[i] = (int *) malloc(25000 * sizeof(int));
        item_target_count[i]=(int *) malloc(25000 * sizeof(int));
        item_line_count[i]=(int *) malloc(25000 * sizeof(int));
        subset[i] = (int **) malloc(25000 * sizeof(int * ));
        item_target[i] = (int **) malloc(25000 * sizeof(int * ));
        item_target_line_count[i]=(int **) malloc(25000 * sizeof(int *));
        item_count[i] = 0;
        int temp_column = ip_column[i];
        for (j=0; j<ip_line_count; j++)
        {
            int temp_glines = g_lines[ip_subset[j]][temp_column];
            int temp_subset = ip_subset[j];
            int temp_item_count = item_count[i] - item_count[i] % 16;
            vec[i] = _mm256_setr_epi32(temp_glines, temp_glines, temp_glines, temp_glines, temp_glines, temp_glines, temp_glines, temp_glines);
            for (k = 0; k < temp_item_count; k+=16)
            {
                int *temp_pos = item[i] + k;
                vec1[i] = _mm256_loadu_si256((__m256i*)temp_pos);
                vec2[i] = _mm256_loadu_si256((__m256i*)(temp_pos + 8));
                vcmp1[i] = _mm256_cmpeq_epi32(vec[i], vec1[i]);
                vcmp2[i] = _mm256_cmpeq_epi32(vec[i], vec2[i]);
                int test1 = _mm256_testz_si256(vcmp1[i], vcmp1[i]);
                int test2 = _mm256_testz_si256(vcmp2[i], vcmp2[i]);
                if(test1 & test2)
                    continue;
                else{
                    if(!test1){
                        int check = _mm256_movemask_epi8(vcmp1[i]);
                        if(check & 1){
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>4) & 1){
                            k+=+1;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>8) & 1){
                            k+=2;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>12) & 1){
                            k+=3;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>16) & 1){
                            k+=4;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>20) & 1){
                            k+=5;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>24) & 1){
                            k+=6;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else{
                            k+=7;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                    }
                    else{
                        int check = _mm256_movemask_epi8(vcmp2[i]);
                        if(check & 1){
                            k+=+8;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>4) & 1){
                            k+=+9;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>8) & 1){
                            k+=10;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>12) & 1){
                            k+=11;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>16) & 1){
                            k+=12;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>20) & 1){
                            k+=13;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else if((check>>24) & 1){
                            k+=14;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                        else{
                            k+=15;
                            subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                            subset[i][k][item_line_count[i][k]] = temp_subset;
                            item_line_count[i][k]++;
                            break;
                        }
                    }
                }
            }
            if(k >= temp_item_count){
                for(; k < item_count[i]; k++){
                    if (!(temp_glines ^ item[i][k]))
                    {
                        subset[i][k] = (int *) realloc(subset[i][k],(item_line_count[i][k]+1) * sizeof(int));
                        subset[i][k][item_line_count[i][k]] = temp_subset;
                        item_line_count[i][k]++;
                        break;
                    }
                }
            }
            
            if (k == item_count[i])
            {
                item[i][k] = temp_glines;
                item_count[i]++;
                item_line_count[i][k] = 1;
                subset[i][k] = (int *) malloc(1 * sizeof(int));
                subset[i][k][0] = temp_subset;
                item_target[i][k] = (int *) malloc(25 * sizeof(int));
                item_target_count[i][k] = 0;
                item_target_line_count[i][k]=(int *) malloc(25 * sizeof(int));
            }
            int temp_glines_target = g_lines[temp_subset][g_target];
            int temp_item_target_count = item_target_count[i][k];
            for (l=0;l<temp_item_target_count;l++)
            {
                if (!(temp_glines_target ^ item_target[i][k][l]))
                {
                    item_target_line_count[i][k][l]++;
                    break;
                }
            }
            if (l == item_target_count[i][k])
            {
                item_target[i][k][l] = temp_glines_target;
                item_target_count[i][k]++;
                item_target_line_count[i][k][l]=1;
            }
        }
        
        gain[i] = 0.0;
        
        for (j=0;j<item_count[i];j++)
        {
            item_gain[i] = 1.0;
            if (item_target_count[i][j] != 1)
            {
                for (k=0;k<item_target_count[i][j];k++)
                {
                    float p = (float)item_target_line_count[i][j][k] / item_line_count[i][j];
                    item_gain[i] = item_gain[i] * pow(p, p);
                }
                item_gain[i] = log2f(item_gain[i]);
                item_gain[i] = item_gain[i] / log2f((float)item_target_count[i][j]);
            }
            gain[i] = gain[i] + item_line_count[i][j] * item_gain[i];
        }
        gain[i] = gain[i] / ip_line_count;
    }
    //#pragma omp barrier
    
    for (i=0; i<ip_column_count; i++){
        int j;
        if (n_gain<=gain[i])
        {
            for (j=0;j<n_item_count;j++)
            {
                free(n_subset[j]);
            }
            for (j=0;j<n_item_count;j++)
            {
                free(n_item_target[j]);
                free(n_item_target_line_count[j]);
            }
            if (n_item_count>0)
            {
                free(n_subset);
                free(n_item);
                free(n_item_line_count);
                free(n_item_target);
                free(n_item_target_count);
                free(n_item_target_line_count);
            }
            
            split_column=ip_column[i];
            n_subset=subset[i];
            n_item=item[i];
            n_item_count=item_count[i];
            n_item_line_count=item_line_count[i];
            n_item_target=item_target[i];
            n_item_target_count=item_target_count[i];
            n_item_target_line_count=item_target_line_count[i];
            n_gain=gain[i];
        }
        else
        {
            for (j=0;j<item_count[i];j++)
            {
                free(subset[i][j]);
            }
            free(subset[i]);
            free(item[i]);
            free(item_line_count[i]);
            for (j=0;j<item_count[i];j++)
            {
                free(item_target[i][j]);
                free(item_target_line_count[i][j]);
            }
            free(item_target[i]);
            free(item_target_count[i]);
            free(item_target_line_count[i]);
        }
    }
    
    //HIGHEST GAIN COLUMN IS SET AS NODE AND REST OF THE COLUMNS ARE PASSED FURTHER
    column = (int *) malloc((ip_column_count-1) * sizeof(int));
    int j = 0;
    for (i=0;i<ip_column_count;i++)
    {
        if (split_column==ip_column[i])
        {
            continue;
        }
        column[j]=ip_column[i];
        j++;
    }
    ip_node->column=split_column;
    ip_node->child=(node *)malloc(sizeof(node));
    ip_node->item=(int *)malloc(sizeof(int));
    ip_node->max_target=(int *)malloc(sizeof(int));
    item_flag=0;
    
    //BRANCHING - LOOP THROUGH ALL ITEM OF THE NODE COLUMN
    for (i=0;i<n_item_count;i++)
    {
        //SET BRANCH DETAILS
        ip_node->item_count++;
        ip_node->child=(node *)realloc(ip_node->child,(i-item_flag+1)*sizeof(node));
        ip_node->child[i-item_flag].item_count=0;
        ip_node->item=(int *)realloc(ip_node->item,(i-item_flag+1)*sizeof(int));
        ip_node->item[i-item_flag] = n_item[i];
        
        max_prob=0.0;
        max_target=0;
        //CALCULATE HIGH OCCURING TARGET FOR EACH BRANCH
        for (j=0;j<n_item_target_count[i];j++)
        {
            if (max_prob<((float)n_item_target_line_count[i][j]*100)/n_item_line_count[i])
            {
                max_prob=((float)n_item_target_line_count[i][j]*100)/n_item_line_count[i];
                max_target=j;
            }
        }
        ip_node->max_target=(int *)realloc(ip_node->max_target,(i-item_flag+1)*sizeof(int));
        ip_node->max_target[i-item_flag] = n_item_target[i][max_target];
        //RECURSIVE CALL - TO GROW CHILD NODES DEPTH WISE
        //CHECK IF MULTIPLE BRANCH WITH NO UNCERTAINITY AND HAS FURTHER LEVELS
        if (ip_column_count!=1 && (n_item_target_count[i]>1) && n_gain!=-1.0)
        {
            grow(n_subset[i],column,n_item_line_count[i],ip_column_count-1,&(ip_node->child[i-item_flag]));
        }
        //SINGLE BRANCH, MARK AS END LEAF AND RETURN BACK
        else if (n_item_target_count[i]==1)
        {
            ip_node->child[i-item_flag].column=g_target;
            ip_node->child[i-item_flag].item_count=1;
            ip_node->child[i-item_flag].child=NULL;
            ip_node->child[i-item_flag].item=(int *)malloc(sizeof(int));
            ip_node->child[i-item_flag].item[0] = n_item_target[i][0];
        }
        else
        {
            ip_node->child[i-item_flag].column=g_target;
            ip_node->child[i-item_flag].item_count=1;
            ip_node->child[i-item_flag].child=NULL;
            ip_node->child[i-item_flag].item=(int *)malloc(sizeof(int));
            if (n_gain==-1.0 || n_gain==0)
            {
                ip_node->child[i-item_flag].item[0] = -1;
            }
            else
            {
                ip_node->child[i-item_flag].item[0] = n_item_target[i][max_target];
            }
        }
        free(n_subset[i]);
    }
    //RELEASE ALLOCATED MEMORY
    free(n_subset);
    free(n_item);
    free(n_item_line_count);
    for (i=0;i<n_item_count;i++)
    {
        free(n_item_target[i]);
        free(n_item_target_line_count[i]);
    }
    free(n_item_target);
    free(n_item_target_count);
    free(n_item_target_line_count);
    if (column!=NULL)
    {
        free(column);
    }
}

