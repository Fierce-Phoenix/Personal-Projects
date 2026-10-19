from flask import Flask, flash, redirect, render_template, request, session, jsonify
from flask_session import Session
from helpers import login_required
from werkzeug.security import check_password_hash, generate_password_hash
import os
import sqlite3

class SQL:
    def __init__(self, url):
        self.path = url.replace("sqlite:///", "", 1)

    def execute(self, query, *args):
        connection = sqlite3.connect(self.path)
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        try:
            cursor = connection.execute(query, args)
            command = query.lstrip().split(None, 1)[0].upper()
            if command in ("SELECT", "PRAGMA", "WITH"):
                result = [dict(row) for row in cursor.fetchall()]
            elif command == "INSERT":
                result = cursor.lastrowid
            else:
                result = cursor.rowcount
            connection.commit()
            return result
        except sqlite3.IntegrityError as e:
            connection.rollback()
            raise ValueError(str(e))
        except sqlite3.Error as e:
            connection.rollback()
            raise RuntimeError(str(e))
        finally:
            connection.close()

app = Flask(__name__)

app.config["SESSION_PERMANENT"] = False
app.config["SESSION_TYPE"] = "filesystem"
app.config["SECRET_KEY"] = "CS50Project"
Session(app)

@app.after_request
def after_request(response):
    """Ensure responses aren't cached"""
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    response.headers["Expires"] = 0
    response.headers["Pragma"] = "no-cache"
    return response

db = SQL("sqlite:///project.db")

@app.route("/")
def index():
    rows = db.execute("SELECT DISTINCT make, model, year, fuel_type, co2_gpm, combined_mpg FROM vehicles ORDER BY make, model, year, fuel_type")
    vehicles = [dict(row) for row in rows]
    return render_template("index.html", vehicles = vehicles)

@app.route("/login", methods=["GET", "POST"])
def login():
    """Log user in"""

    # Forget any user_id, but maintain flashed message if present
    session.pop("user_id", None)

    # User reached route via POST (as by submitting a form via POST)
    if request.method == "POST":
        # Ensure username was submitted
        if not request.form.get("username"):
            flash("Must provide Username")
            return redirect("/login")

        # Ensure password was submitted
        elif not request.form.get("password"):
            flash("Must provide Password")
            return redirect("/login")

        # Query database for username
        rows = db.execute(
            "SELECT * FROM users WHERE username = ?", request.form.get("username")
        )

        # Ensure username exists and password is correct
        if len(rows) != 1 or not check_password_hash(
            rows[0]["password_hash"], request.form.get("password")
        ):
            flash("Invalid Username and/or Password")
            return redirect("/login")

        # Remember which user has logged in
        session["user_id"] = rows[0]["user_id"]

        # Redirect user to home page
        return redirect("/")

    # User reached route via GET (as by clicking a link or via redirect)
    else:
        return render_template("login.html")

@app.route("/register", methods=["GET", "POST"])
def register():
    """Register user"""
    if request.method == "POST":
        if not request.form.get("username"):
            flash("Must provide Username")
            return redirect("/register")
        elif not request.form.get("password"):
            flash("Must provide Password")
            return redirect("/register")
        elif not request.form.get("confirmation"):
            flash("Must re-enter Password")
            return redirect("/register")

        #check if username exists
        username_check = db.execute("SELECT * FROM users WHERE username = ?", request.form.get("username"))
        if not username_check:
            if (request.form.get("password") == request.form.get("confirmation")):
                try:
                    db.execute("INSERT INTO users (username, password_hash) VALUES (?, ?)", request.form.get(
                        "username"), generate_password_hash(request.form.get("password")))
                    flash("Successfully registered")
                    return redirect("/login")
                except (ValueError, RuntimeError):
                    flash("Username already exists")
                    return redirect("/register")

            else:
                flash("Password does not match")
                return redirect("/register")
        else:
            flash("Username already exists")
            return redirect("/register")

    else:
        return render_template("register.html")

@app.route("/logout")
def logout():
    """Log user out"""

    try:
        db.execute("DELETE FROM compare WHERE user_id = ?", session["user_id"])
    except KeyError:
        pass

    # Forget any user_id BUT DO NOT ERASE THE FLASH MESSAGES
    session.pop("user_id", None)

    # Redirect user to login form
    return redirect("/login")

@app.route("/reset_password", methods=["GET", "POST"])
@login_required
def reset_password():
    if request.method == "POST":
        if not request.form.get("current_password"):
            flash("Must provide Current Password")
            return redirect("/reset_password")
        elif not request.form.get("new_password"):
            flash("Must provide New Password")
            return redirect("/reset_password")
        elif not request.form.get("confirmation"):
            flash("Must re-enter New Password")
            return redirect("/reset_password")

        current_hashed_password = db.execute(
            "SELECT password_hash FROM users WHERE user_id = ?", session["user_id"])

        if check_password_hash(current_hashed_password[0]["password_hash"], request.form.get("current_password")) == False:
            flash("Current Password does not match")
            return redirect("/reset_password")

        if (request.form.get("new_password") == request.form.get("confirmation")):
            db.execute("UPDATE users SET password_hash = ? WHERE user_id = ?", generate_password_hash(
                request.form.get("new_password")), session["user_id"])
            flash("Password successfully reset")
            return redirect("/logout")
        else:
            flash("New Password does not match")
            return redirect("/reset_password")
    else:
        return render_template("reset_password.html")

@app.route("/search")
def search_render():
    # Always get the full list of makes for the first dropdown
    makes = db.execute("SELECT DISTINCT make FROM vehicles ORDER BY make")
    return render_template("search.html", makes=makes, selected_make=None,
                            models=[], selected_model=None,
                            years=[], selected_year=None)


@app.route("/search/makes")
def search_makes():
    selected_make = request.args.get("make")

    makes = db.execute("SELECT DISTINCT make FROM vehicles ORDER BY make")
    models = db.execute("SELECT DISTINCT model FROM vehicles WHERE make = ? ORDER BY model", selected_make)

    return render_template("search.html", makes=makes, selected_make=selected_make,
                            models=models, selected_model=None,
                            years=[], selected_year=None)


@app.route("/search/models")
def search_models():
    selected_make = request.args.get("make")
    selected_model = request.args.get("model")

    makes = db.execute("SELECT DISTINCT make FROM vehicles ORDER BY make")
    models = db.execute("SELECT DISTINCT model FROM vehicles WHERE make = ? ORDER BY model", selected_make)
    years = db.execute("SELECT DISTINCT year FROM vehicles WHERE make = ? AND model = ? ORDER BY year",
                        selected_make, selected_model)

    return render_template("search.html", makes=makes, selected_make=selected_make,
                            models=models, selected_model=selected_model,
                            years=years, selected_year=None)


@app.route("/search/submit")
def search_submit():

    selected_id = request.args.get("car_id")
    if selected_id:
        results = db.execute("SELECT * FROM vehicles WHERE car_id = ?", selected_id)
    else:
        selected_make = request.args.get("make")
        selected_model = request.args.get("model")
        selected_year = request.args.get("year")

        results = db.execute("SELECT * FROM vehicles WHERE make = ? AND model = ? AND year = ?",
                            selected_make, selected_model, selected_year)

    return render_template("searched.html", results = results)

@app.route("/shortlisted_vehicles", methods = ["POST", "GET"])
@login_required
def shortlist():
    if request.method == "POST":
        selected_id = request.form.get("car_id")
        db.execute("INSERT OR IGNORE INTO shortlist (user_id, car_id) VALUES (?,?)", session["user_id"], selected_id)
        flash("Successfully added to shortlist")
        return redirect(request.referrer)
    else:
        cars = db.execute("SELECT * FROM vehicles WHERE car_id IN (SELECT car_id FROM shortlist WHERE user_id = ?) ORDER BY make, model, year, combined_mpg, co2_gpm",session["user_id"])
        return render_template("shortlisted_vehicles.html", cars = cars)

@app.route("/shortlisted_vehicles/remove", methods = ["POST"])
@login_required
def shortlist_remove():
    selected_id = request.form.get("car_id")
    db.execute("DELETE FROM shortlist WHERE user_id = ? AND car_id = ?", session["user_id"], selected_id)
    flash("Successfully removed from shortlist")
    return redirect(request.referrer)

@app.route("/compare", methods = ["POST", "GET"])
@login_required
def compare():
    if request.method == "POST":
        selected_id = request.form.get("car_id")
        db.execute("INSERT OR IGNORE INTO compare (user_id, car_id) VALUES (?,?)", session["user_id"], selected_id)
        flash("Successfully added to comparison list")
        return redirect(request.referrer)
    else:
        cars = db.execute("SELECT * FROM vehicles WHERE car_id IN (SELECT car_id FROM compare WHERE user_id = ?)",session["user_id"])
        return render_template("compare.html", cars = cars)

@app.route("/compare/remove", methods = ["POST"])
@login_required
def compare_remove():
    selected_id = request.form.get("car_id")
    db.execute("DELETE FROM compare WHERE user_id = ? AND car_id = ?", session["user_id"], selected_id)
    flash("Successfully removed from comparison list")
    return redirect(request.referrer)

@app.route("/delete_account", methods = ["POST","GET"])
@login_required
def delete_account():
    confirm = False
    if request.method == "POST":
        if not request.form.get("password"):
            flash("Must provide Password")
            return redirect("/delete_account")
        elif not request.form.get("confirmation"):
            flash("Must re-enter Password")
            return redirect("/delete_account")

        if request.form.get("password") != request.form.get("confirmation"):
            flash("Passwords do not match")
            return redirect("/delete_account")

        correct_password = db.execute("SELECT password_hash FROM users WHERE user_id = ?", session["user_id"])
        print(correct_password)
        print(correct_password[0])
        print(correct_password[0]["password_hash"])
        # Ensure password is correct
        if not check_password_hash(
            correct_password[0]["password_hash"], request.form.get("password")
        ):
            flash("Invalid Password")
            return redirect("/delete_account")
        else:
            confirm = True
            return render_template("delete_account.html", confirm = confirm)
    else:
        return render_template("delete_account.html", confirm = confirm)

@app.route("/delete_account/confirm", methods = ["POST"])
@login_required
def delete_account_confirm():
    db.execute("DELETE FROM shortlist WHERE user_id = ?", session["user_id"])
    db.execute("DELETE FROM compare WHERE user_id = ?", session["user_id"])
    db.execute("DELETE FROM users WHERE user_id = ?", session["user_id"])

    flash("Your account has been deleted")
    return redirect("/logout")