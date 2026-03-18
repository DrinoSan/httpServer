#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Example of freeElem for char*
void string_free( void** elem )
{
   // clang-format off
   // i dereference the elem pointer. This gives me the the value inside data
   // which itself is a pointer to the heap allocated string elem pointer is
   // provided by sand_list_free
   //
   // With other words char** says its a pointer to a
   // pointer meaning elem -> field in data -> heap where string lives
   // the dereference give me the value of elem it points to which is field in
   // data which is the address of the heap
   // clang-format on
   free( *( char** ) elem );
}

// Implementation of list
typedef struct
{
   void*  data;                   // generic data
   size_t size;                   // number of elements inside list
   size_t capacity;               // Number of potential elements
   size_t type_size;              // Size of single element
   void ( *freeElem )( void* );   // Needed if we hold char* elements which are
                                  // pointers to allocated space
} sand_list;

//-----------------------------------------------------------------------------
/// Function to push element e to the end of list l
/// @param: l sand_list is the destination
/// @param: e generic element defined by list type_size which will be pushe to
/// the end of the list
static sand_list* sand_list_push_back( sand_list* l, void* e )
{
   if ( l->size < l->capacity )
   {
      // Casting to char* so i can calculate easier
      char* p = ( char* ) l->data;

      // Getting pointer offset where to insert
      p += l->size * l->type_size;

      memcpy( ( void* ) p, e, l->type_size );

      l->size++;
      return l;
   }

   // Fresh list
   if ( l->size == 0 )
   {
      // Not checking for calloc return because if this fails everything should
      // burn
      l->capacity = 4;
      l->data     = calloc( l->capacity, l->type_size );

      // Casting to char* so i can calculate easier
      char* p = ( char* ) l->data;

      // Getting pointer offset where to insert
      p += l->size * l->type_size;

      memcpy( ( void* ) p, e, l->type_size );

      l->size++;
      return l;
   }

   // Need to extend the list and potentially copy stuff
   l->capacity *= 2;   // Double the memory
   // Realloc does free the old pointer if a new block is captured
   l->data = realloc( l->data, l->capacity * l->type_size );

   if ( l->data == NULL )
   {
      printf( "[sand_lib] Failed to increase memory block in push_back\n" );
      exit( 1 );
   }

   // Casting to char* so i can calculate easier
   char* p = ( char* ) l->data;

   // Getting pointer offset where to insert
   p += l->size * l->type_size;

   memcpy( ( void* ) p, e, l->type_size );

   l->size++;

   return l;
}

//-----------------------------------------------------------------------------
/// Function to remove last element from list
/// If list is empty 0 is returned
/// If out element is set it the popped element will be written to it
static int sand_list_pop_back( sand_list* l, void* out_element )
{
   if ( l->size == 0 )
   {
      return 0;
   }

   if ( out_element == NULL )
   {
      l->size--;
      return 1;
   }

   // Need this to be able to do correct pointer arithmetic
   l->size--;
   char* src = ( char* ) l->data + l->size * l->type_size;

   // Writing into out_element
   memcpy( out_element, src, l->type_size );

   return 1;
}

//-----------------------------------------------------------------------------
/// Function to get a copy of element at index
/// Copy is written to out_element
/// Returns 0 if index is out of bounds otherwise 1
static int sand_list_get( sand_list* l, int index, void* out_element )
{
   if ( ( size_t ) index >= l->size )
   {
      printf( "[sand_list] out of bounds, index %d not valid list size %zu\n",
              index, l->size );
      return 0;
   }

   char* src = ( char* ) l->data + index * l->type_size;
   memcpy( out_element, src, l->type_size );

   return 1;
}

//-----------------------------------------------------------------------------
/// Function to get a pointer to element at index
/// Returns pointer to element if sucessfull otherweise NULL
static void* sand_list_at( sand_list* l, int index )
{
   if ( ( size_t ) index >= l->size )
   {
      printf( "[sand_list] out of bounds, index %d not valid list size %zu\n",
              index, l->size );
      return NULL;
   }

   return ( char* ) l->data + index * l->type_size;
}

//-----------------------------------------------------------------------------
static sand_list* sand_list_clear( sand_list* l )
{
   if ( l == NULL )
   {
      printf( "[sand_list] given list is a NULL pointer" );
      return NULL;
   }

   l->size = 0;
   return l;
}

//-----------------------------------------------------------------------------
static sand_list* sand_list_shrink_to_fit( sand_list* l )
{
   if ( !l || l->size == l->capacity )
   {
      // Nothing to shrink
      return l;
   }

   l->data = realloc( l->data, l->size * l->type_size );
   // I gess shrinking cant fail..
   l->capacity = l->size;
   return l;
}

//-----------------------------------------------------------------------------
/// Function to create a empty list
/// If memory allocation fails there is no need to continue
/// @param: type_size needed for generic work
/// @return: Pointer to empty list
static sand_list* sand_list_create( size_t type_size,
                                    void ( *freeElem )( void* ) )
{
   sand_list* l = calloc( 1, sizeof( sand_list ) );
   if ( l == NULL )
   {
      printf( "[sand_list] cant init allocate memory for list\n" );
      exit( 1 );
   }

   l->size      = 0;
   l->capacity  = 0;
   l->type_size = type_size;
   l->freeElem  = freeElem;

   return l;
}

//-----------------------------------------------------------------------------
static void sand_list_free( sand_list* l )
{
   if ( l->freeElem != NULL )
   {
      for ( int32_t i = 0; i < l->size; i++ )
      {
         // elem points to a char* inside data, freeElem dereferences it to get
         // the actual string pointer and frees it
         void* elem = ( char* ) l->data + i * l->type_size;
         l->freeElem( ( void** ) elem );
      }
   }

   printf( "Killed List \n" );
   free( l->data );
}
