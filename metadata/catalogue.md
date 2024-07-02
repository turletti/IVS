## SWHAPPE catalogue.md Template

Here we propose a template for catalogue<span>.md for SWHAPPE.

Each Item in the `raw_materials` and `additional_materials` folders of Workbench should have a corresponding record on the `catalogue.md` with the structure below..

Please note that:
* Name and Surname of actors should be linked to their paragraph in [actors.md](./actors.md) file;
* Items should be linked to the file [inside the raw materials repository](./raw_matherials/) or [inside the additional materials repository](./additional_matherials/);
* On the [second part of the Catalogue](./catalogue.md#SW_NAME-Catalogue-Tree) should be copied the result of the command `tree -a raw_materials additional_materials ` ;
* *Notes:* are optional; they should contains `Additional materials.` for additional materials
* *Warehouse:* is optional - should be used only when a physical warehouse is used to store material taken from the *origin*; .

Example of Actor link:
~~~
[Name Surname](./actors.md#name-surname)
~~~
Example of Item links:
~~~
[Item Name](./raw_materials/example_file.zip)
[Additional Materials Item Name](./additional_materials/example.ppt)
~~~


# SW_NAME Catalogue


* **[ivs1.5.tgz](./raw_materials/ivs1.5.tgz)**
  * *Origin: Author personal archives
  * *Warehouse: Author personal archives
  * *Authors:* [Thierry Turletti](./actors.md#name-surname)
  * *Date:* 1992/07/08 (circa) 
  * *Collectors:* [Thierry Turletti](./actors.md#name-surname)
  * *Description: Source code of version 1.5 of IVS (Tar Gzip)
  * *Notes: 
  
* **[ivs1.8.tgz](./raw_materials/ivs1.8.tgz)**
  * *Origin: Author personal archives
  * *Warehouse: Author personal archives
  * *Authors:* [Thierry Turletti](./actors.md#name-surname)
  * *Date:* 1992/09/25 (circa) 
  * *Collectors:* [Thierry Turletti](./actors.md#name-surname)
  * *Description: Source code of version 1.8 of IVS (Tar Gzip)
  * *Notes: 
  
* **[ivs3.1.tgz](./raw_materials/ivs3.1.tgz)**
  * *Origin: Author personal archives
  * *Warehouse: Author personal archives
  * *Authors:* [Thierry Turletti](./actors.md#name-surname)
  * *Date:* 1993/06/10 (circa) 
  * *Collectors:* [Thierry Turletti](./actors.md#name-surname)
  * *Description: Source code of version 3.1 of IVS (Tar Gzip)
  * *Notes: 
  
* **[ivs3.6a.tgz](./raw_materials/ivs3.6a.tgz)**
  * *Origin: Author personal archives
  * *Warehouse: Author personal archives
  * *Authors:* [Thierry Turletti](./actors.md#name-surname)
  * *Date:* 1995/07/01 (circa) 
  * *Collectors:* [Thierry Turletti](./actors.md#name-surname)
  * *Description: Source code of version 3.6a of IVS (Tar Gzip)
  * *Notes: 
  
# SW_NAME Catalogue Tree


result of `tree -a raw_materials additional_materials` command.
