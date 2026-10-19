## Eco Cars
Eco Cars is a web application that allows prospective vehicle buyers to compare the efficiency and emmissions of vehicles in one place. Each vehicle can be shortlisted and compared against other vehicles. They can then use the information made available to them to make an informed purchasing decision. Users can also use the homepage to calculate carbon emmissions and fuel consumption by providing how many miles they intend to drive.

The app was developed using the Codespaces plugin in Visual Studio Code.

The project is built using Python, Flask and Javascript. It uses SQLite3 for database management. Elements of Bootstrap have been used to make this app more interactive and user friendly.

The vehicle emmisions data is provided by https://www.fueleconomy.gov/feg/download.shtml. It comprises of vehicle data of models sold in the United States which were sold from 1984 to 2026.

## Youtube demo
https://youtu.be/3Vz7b6RM_mM

## Source Files
app.py - Main source file comprising of functions and routes for the app.
helpers.py - Custom helper function for users requiring login. This file is directly based on the helpers.py provided to us to be used in the CS50 Finance problem set.

## Explaing the app

# Homepage
The homepage is a calculator wherein users can calculate estimated emmissions and fuel consumption by inputing their mileage. All required data is selected from the SQL database and sent to the html. Javascript is used on the front end to convert the data into json and the webpage updates dynamically every time the user interacts with it.

# Search
This uses a different approach to search for vehicles as compared to the homepage. Initially, a template is rendered comprising only of the selected make, followed by the model and the year. Precautions have been set in to ensure that user does not submit a vehicle which does not exist on the database. Once the user submits his/her query, the webpage renders a table of the selected vehicles.

# Compare
This allows users to temporarily add vehicles to a comparison list and determine which vehicle they should shortlist. The comparison list gets reset every time the user logs out from the website.

# Shortlisted Vehicles
Similar to 'Compare', Shortlisted Vehicles allows users to add vehicles to a list which they can use to make a conscious purchasing decision. Unlike 'Compare', this list is permanent and vehicles can only be removed from the list if the user manually removes it from the list.

## Register, Login, Reset Password and Delete Account
# Register
Users can set up a new account by selecting their own username and password combinations. Every username is unique and the user is notified if the username they wish to use already exists. Passwords are hashed and saved to the database

# Login
The users table in the project database is accessed to allow users to login to the website. Passwords from the database are decrypted and then verified.

# Reset Password
This feature allows users to reset their password if they feel the need to do so. Users are required to input their previous password and reverify their new one.

# Delete Account
This gives users to delete their accoount and all information associated with their account if they wish to. When the account is set up for deletion, all the user associated data is individually cleared from every table in the database including any credentials.


## Acknowledgments
I used parts of the CS50 Finance assignment (specifically those associated with logging in, logging out and the navbar design) for this project, modifying them to some extent for this project. I also used Claude (Anthropic) to help debug parts of this project, specifically the cascading dropdown logic used in index.html and the approach for passing vehicle data from Flask to JavaScript using
Jinja's `tojson` filter.
