//Barış Başmak
//2016400087
//Compiling
//Working

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <mpi.h>
#include <stdlib.h>

using namespace std ; 

int main(int argc, char * argv[] )
{

	int id;
	int ierr;
	int processor_count ;   //how many processors there are  
    ierr = MPI_Init ( &argc, &argv );

	if ( ierr != 0 )
    {
    	cout << "\n";
    	cout << "MULTITASK_MPI - Fatal error!\n";
    	cout << "  MPI_Init returned ierr = " << ierr << "\n";
    	exit ( 1 );
    }

	ierr = MPI_Comm_rank ( MPI_COMM_WORLD, &id );

	ierr = MPI_Comm_size ( MPI_COMM_WORLD, &processor_count );



// Master processor
	if(id == 0)
	{
		string arguments[argc] ;
		for(int i = 0 ; i < argc ; i++){
		    arguments[i] = argv[i];
		}

		
		string input_file = arguments[1];
		string output_file = arguments[2];
		string iteration = arguments[3];
	
		stringstream streamer2(iteration);
		int iteration_count;
		
		streamer2 >> iteration_count;
	 
		int tokens[360][360] ; 

		ifstream myfile (input_file);
	  	
	  	if (myfile.is_open()){


	  	  for(int i = 0 ; i < 360 ; i++){	  	  		
	  	    for(int j = 0 ; j < 360 ; j++){				
		    		int q;
					myfile >> q;
		       		tokens[i][j] = q ; 
		       		
	    	    } 

		  	  }
	    		myfile.close();
		}

        ///////////sending data to worker processors ! 
        int row_per_worker = 360 / (processor_count-1) ; 
        int width = 360 ; 
        int data_points_per_worker = width * row_per_worker ; 

        for(int i = 1 ; i < processor_count ; i++)
        {
            MPI_Send(&tokens [ (i - 1) * row_per_worker ][ 0 ] , data_points_per_worker, MPI_INT , i , 0, MPI_COMM_WORLD ) ; 
        }
        //Data distributed to workers 


        //Receiving the data back and wirting to file
        int received_back = 0 ;
        MPI_Status status ;
        while(received_back < ( processor_count - 1)  )
        {
            int receive[row_per_worker][width] ; 

            MPI_Recv(&receive[0][0] , data_points_per_worker, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 
            
            int source = status.MPI_SOURCE ;
            int starting_index = (source - 1) * row_per_worker ; 

            for(int i = 0 ; i < row_per_worker ; i++) 
            {
                for(int j = 0 ; j < 360 ; j++)
                {
                    tokens[ starting_index + i ][j] = receive[i][j] ;
                }
            }

            received_back ++ ; 
        }

    //Writing to the outputfile 
        ofstream new_file( output_file ) ;
	      for(int i = 0 ; i < 360 ; i++)
	      {
	      	for(int j = 0 ; j < 360 ; j++ )
	      	{
	      		if( j != 360)
	      			new_file<<tokens[i][j]<<" ";
	      		else
	      			new_file<<tokens[i][j];

	      	}
	      	new_file<<endl;
          }
        MPI_Finalize ( ) ;
    }
    else if(id != 0)
    {   
        string arguments[argc] ;
		for(int i = 0 ; i < argc ; i++)
        {
		    arguments[i] = argv[i];
		}

        int iteration_wanted = stoi(arguments[3]); // desired iteration count 

        int width = 360 ; //width of the map
        int worker_count = processor_count - 1 ; //Worker processor count  
        int my_row_count = 360 / worker_count ;  // row count per worker 
        int my_data_points = my_row_count * width ;    // total number of cells per worker 

        int work_data [ my_row_count + 2 ][ width ] ;
        int send_data [ my_row_count ][ width ] ;

        int ierr;
        MPI_Status status ; 
        //Receiving the map data from P0 
        ierr = MPI_Recv(&send_data[0][0], my_data_points, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status) ; 

        if(ierr != 0)
        {
            cout<<"Error when receiving data from P0   This is processor P "<<id <<endl ;
        }

        //Copying the matrix for more convenient access 
        for(int i = 0  ;  i < my_row_count ; i++)
        {
            for(int j = 0 ; j < width ; j++) 
            {
                work_data[i+1][j] = send_data[i][j] ;
            }
        }
        //Receiving data from P0 
        

        //COMMUNNICATION BETWEEN PROCESSES 

//send_datadan atmalik 
        int bottom_send_index = my_row_count-1 ; 
        int top_send_index = 0 ; 

//worker_datasi icin
        int top_recv = 0 ; 
        int bottom_recv = my_row_count+1; 
       int current_iteration ; 
       while(iteration_wanted != current_iteration)
       {


            if( id % 2 == 1)
            {
                if(id != worker_count)
                {//Asagiya atis ve asagidan recv
                    MPI_Send(&send_data[bottom_send_index][0], width, MPI_INT, (id + 1), 0, MPI_COMM_WORLD );
                    MPI_Recv(&work_data[bottom_recv][0], width , MPI_INT, id + 1  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ;
                }
                //Project assumption --> EVEN NUMBER OF WORKER PROCESSORS 
                if(id != 1)
                {
                    MPI_Send(&send_data[top_send_index][0] , width, MPI_INT , id -1 , 0 , MPI_COMM_WORLD);
                    MPI_Recv(&work_data[top_recv][0], width , MPI_INT, id - 1  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ;
                }
                else 
                {
                    MPI_Send(&send_data[top_send_index][0] , width, MPI_INT , worker_count , 0 , MPI_COMM_WORLD);
                    MPI_Recv(&work_data[top_recv][0], width , MPI_INT, worker_count  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ;   
                }
            }

            else if(id % 2 == 0)
            {
                MPI_Recv(&work_data[top_recv][0],  width , MPI_INT, id - 1  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ; 
                MPI_Send(&send_data[top_send_index][0] , width, MPI_INT , id -1 , 0 , MPI_COMM_WORLD);

                if(id != worker_count)
                {
                    MPI_Recv(&work_data[bottom_recv][0],  width , MPI_INT, id + 1  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ; 
                    MPI_Send(&send_data[bottom_send_index][0] , width, MPI_INT , id + 1 , 0 , MPI_COMM_WORLD);
                }else
                {
                    MPI_Recv(&work_data[bottom_recv][0],  width , MPI_INT, 1  ,MPI_ANY_TAG, MPI_COMM_WORLD, &status) ; 
                    MPI_Send(&send_data[bottom_send_index][0] , width, MPI_INT , 1 , 0 , MPI_COMM_WORLD);  
                }
                
            }
            
            int helper[8][2] = {-1, -1, 0, -1, 1, -1, -1, 0, 1, 0 , -1 , 1, 0, 1, 1, 1} ;
            for(int i = 1 ; i < my_row_count + 1 ; i++)
            {
                for(int j = 0 ; j < width ; j++)
                {
                    int current = work_data[i][j] ; 
                    int count = 0 ; 
                    bool rightmost = ( j == 359 ) ; 
                    bool leftmost  = ( j == 0 )   ;

                    if(rightmost)
                    {
                        
                        for(int x = 0 ; x < 8 ; x++)
                        {
                            if( x < 5 && work_data[i + helper[x][0] ][ j + helper[x][1] ] == 1)
                                count++ ; 

                            
                        }
                        if (work_data[i - 1][0] == 1)  
                                count ++; 
                        if( work_data[i][0] == 1)
                            count++ ;
                        if(work_data[i + 1][0] == 1)
                            count++ ; 
                    }
                    if(leftmost)
                    {
                        
                        for(int x = 0 ; x < 8 ; x++)
                        {
                            if( x > 2 && work_data[i + helper[x][0] ][ j + helper[x][1] ] == 1)
                                count++ ;
                            
                        }
                        if (work_data[i - 1][359] == 1)  
                                count ++; 
                        if( work_data[i][359] == 1)
                            count++ ;
                        if(work_data[i + 1][359] == 1)
                            count++ ; 
                    }
                    if(!leftmost && !rightmost)
                    {

                        for(int x = 0 ; x < 8 ; x++)
                        {
                            if( work_data[i + helper[x][0] ][ j + helper[x][1] ] == 1)
                            
                                count++ ;
                        } 
                    }

                    if(count < 2 )
                    {
                        send_data[i-1][j] = 0 ;
                    }
                    if(count > 3)
                    {
                        send_data[i-1][j] = 0 ;
                    }
                    if(count == 3 && send_data[i-1][j] == 0)
                    {
                        send_data[i-1][j] = 1 ;
                    }
                

                }



            }
            for(int k = 0 ; k < my_row_count ; k++ )
            {
                for(int m = 0 ; m < width ; m++)
                {
                    work_data[k+1][m] = send_data[k][m];
                }
            }




            current_iteration++ ; 
        }


        MPI_Send(&send_data[0][0], my_data_points, MPI_INT, 0, 0, MPI_COMM_WORLD) ;


        MPI_Finalize ( ) ; 
    }






    return 0 ;
}

