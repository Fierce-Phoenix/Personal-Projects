# Eco Cars

Eco Cars is a web application that allows prospective vehicle buyers to compare the efficiency and emissions of vehicles in one place. Vehicles can be shortlisted and compared against one another, enabling users to make an informed purchasing decision. The homepage also provides a calculator that estimates carbon emissions and fuel consumption based on the number of miles a user intends to drive.

The application was developed using the Codespaces extension in Visual Studio Code. It is built with Python, Flask and JavaScript, uses SQLite3 for database management, and uses Bootstrap components to provide a responsive and user-friendly interface.

Vehicle data is sourced from [fueleconomy.gov](https://www.fueleconomy.gov/feg/download.shtml) and covers models sold in the United States between 1984 and 2026.

## Video Demo

https://youtu.be/3Vz7b6RM_mM

## Source Files

- `app.py` – The main application file, containing the application's routes and functions.
- `helpers.py` – A custom helper providing the `login_required` decorator for routes that require an authenticated user. It is based on the `helpers.py` supplied with the CS50 Finance problem set.

## Application Overview

### Homepage

The homepage is a calculator that estimates emissions and fuel consumption from a mileage value entered by the user. The required vehicle data is retrieved from the database and passed to the template, where JavaScript converts it to JSON. The page then updates dynamically as the user interacts with it.

### Search

Search uses a different approach from the homepage. The user first selects a make, then a model, then a year, with each selection rendering the next set of options. This ensures that only vehicles present in the database can be submitted. Once a search is submitted, a table of the matching vehicles is displayed.

### Compare

The comparison list allows users to temporarily gather vehicles side by side in order to decide which to shortlist. The list is cleared each time the user logs out.

### Shortlisted Vehicles

The shortlist allows users to save vehicles for later consideration. Unlike the comparison list, the shortlist is persistent: a vehicle remains on it until the user removes it.

## Account Management

### Register

Users create an account by choosing a username and password. Usernames must be unique, and the user is notified if the chosen username is already in use. Passwords are hashed before being stored in the database.

### Login

Credentials are checked against the `users` table in the project database. The submitted password is verified against the stored password hash.

### Reset Password

Users may change their password by entering their current password and confirming a new one.

### Delete Account

Users may permanently delete their account. Upon confirmation, all data associated with the account, including shortlisted vehicles, comparison entries and login credentials, is removed from the database.

## Acknowledgments

Portions of the CS50 Finance problem set, specifically the login and logout functionality and the navigation bar design, were adapted for this project.
